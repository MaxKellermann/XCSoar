/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

}
*/


#include "stdafx.h"
#include <Aygshell.h>

#include "XCSoar.h"

#include "WindowControls.h"
#include "Statistics.h"
#include "Externs.h"
#include "McReady.h"
#include "dlgTools.h"
#include "InfoBoxLayout.h"

extern void DrawJPG(HDC hdc, RECT rc);
#include "VOIMAGE.h"

extern TCHAR szRegistryHomeWaypoint[];
extern TCHAR szRegistryWayPointFile[];

static int page=0;
static WndForm *wf=NULL;
static WndListFrame *wDetails=NULL;
static WndOwnerDrawFrame *wDetailsEntry = NULL;
static WndFrame *wInfo=NULL;
static WndFrame *wCommand=NULL;
static WndOwnerDrawFrame *wImage=NULL;
static BOOL hasimage1 = false;
static BOOL hasimage2 = false;
static CVOImage jpgimage1;
static CVOImage jpgimage2;
static TCHAR path_modis[100];
static TCHAR path_fname2[] = TEXT("\\Program Files\\omap\\ersa-benalla.jpg");
static TCHAR szWaypointFile[MAX_PATH] = TEXT("\0");
static TCHAR Directory[MAX_PATH];

#define MAXLINES 100
static int LineOffsets[MAXLINES];
static int DrawListIndex=0;
static int nTextLines=0;

static void NextPage(int Step){
  page += Step;

  if ((!WayPointList[SelectedWaypoint].Details)&&(page==1)) {
    page++;
  }

  if (!jpgimage1 && !jpgimage2 && page > 2)
    page = 0;
  if (jpgimage1 && !jpgimage2 && page > 3)
    page = 0;
  if (jpgimage1 && jpgimage2 && page > 4)
    page = 0;
  if (!jpgimage1 && !jpgimage2 && page < 0)
    page = 2;
  if (jpgimage1 && !jpgimage2 && page < 0)
    page = 3;
  if (jpgimage1 && jpgimage2 && page < 0)
    page = 4;

  wInfo->SetVisible(page == 0);
  wDetails->SetVisible(page == 1);
  wCommand->SetVisible(page == 2);
  wImage->SetVisible(page > 2);

  if (page==1) {
    wDetails->ResetList();
    wDetails->Redraw();
  }

}


static void OnPaintDetailsListItem(WindowControl * Sender, HDC hDC){

  if (DrawListIndex < nTextLines){
    TCHAR* text = WayPointList[SelectedWaypoint].Details;
    int nstart = LineOffsets[DrawListIndex];
    int nlen;
    if (DrawListIndex<nTextLines-1) {
      nlen = LineOffsets[DrawListIndex+1]-LineOffsets[DrawListIndex]-1;
      nlen--;
    } else {
      nlen = _tcslen(text+nstart);
    }
    if (_tcscmp(text+nstart+nlen-1,TEXT("\r"))==0) {
      nlen--;
    }
    if (_tcscmp(text+nstart+nlen-1,TEXT("\n"))==0) {
      nlen--;
    }
    if (nlen>0) {
      ExtTextOut(hDC, 2*InfoBoxLayout::scale, 2*InfoBoxLayout::scale,
		 ETO_OPAQUE, NULL,
		 text+nstart,
		 nlen, 
		 NULL);
    }
  }
}


static void OnDetailsListInfo(WindowControl * Sender, WndListFrame::ListInfo_t *ListInfo){
  if (ListInfo->DrawIndex == -1){
    ListInfo->ItemCount = nTextLines-1;
  } else {
    DrawListIndex = ListInfo->DrawIndex+ListInfo->ScrollIndex;
  }
}



static void OnNextClicked(WindowControl * Sender){
  NextPage(+1);
}

static void OnPrevClicked(WindowControl * Sender){
  NextPage(-1);
}

static void OnCloseClicked(WindowControl * Sender){
  wf->SetModalResult(mrOK);
}

static int FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam){

  switch(wParam & 0xffff){
    case VK_LEFT:
    case '6':
      SetFocus(((WndButton *)wf->FindByName(TEXT("cmdPrev")))->GetHandle());
      NextPage(-1);
      //((WndButton *)wf->FindByName(TEXT("cmdPrev")))->SetFocused(true, NULL);
    return(0);
    case VK_RIGHT:
    case '7':
      SetFocus(((WndButton *)wf->FindByName(TEXT("cmdNext")))->GetHandle());
      NextPage(+1);
      //((WndButton *)wf->FindByName(TEXT("cmdNext")))->SetFocused(true, NULL);
    return(0);
  }
  return(1);
}


static void OnGotoClicked(WindowControl * Sender){
  LockTaskData();
  FlyDirectTo(SelectedWaypoint);
  UnlockTaskData();
  wf->SetModalResult(mrOK);
}

static void OnReplaceClicked(WindowControl * Sender){
  LockTaskData();
  ReplaceWaypoint(SelectedWaypoint);
  RefreshTask();
  UnlockTaskData();
  wf->SetModalResult(mrOK);
}

static void OnNewHomeClicked(WindowControl * Sender){
  LockTaskData();
  HomeWaypoint = SelectedWaypoint;
  SetToRegistry(szRegistryHomeWaypoint, HomeWaypoint);
  RefreshTask();
  UnlockTaskData();
  wf->SetModalResult(mrOK);
}


static void OnTeamCodeClicked(WindowControl * Sender){
  TeamCodeRefWaypoint = SelectedWaypoint;
  SetToRegistry(szRegistryTeamcodeRefWaypoint, TeamCodeRefWaypoint);
  wf->SetModalResult(mrOK);
}


static void OnInserInTaskClicked(WindowControl * Sender){
  LockTaskData();
  InsertWaypoint(SelectedWaypoint);
  RefreshTask();
  UnlockTaskData();
  wf->SetModalResult(mrOK);
}

static void OnAppendInTaskClicked(WindowControl * Sender){
  LockTaskData();
  InsertWaypoint(SelectedWaypoint, true);
  RefreshTask();
  UnlockTaskData();
  wf->SetModalResult(mrOK);
}


static void OnRemoveFromTaskClicked(WindowControl * Sender){
  LockTaskData();
  RemoveWaypoint(SelectedWaypoint);
  RefreshTask();
  UnlockTaskData();
  wf->SetModalResult(mrOK);
}

static void OnImagePaint(WindowControl * Sender, HDC hDC){

  if (page == 3)
    jpgimage1.Draw(hDC, 0, 0, -1, -1);

  if (page == 4)
    jpgimage2.Draw(hDC, 0, 0, -1, -1);

}


static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnNextClicked),
  DeclearCallBackEntry(OnPrevClicked),
  DeclearCallBackEntry(OnPaintDetailsListItem),
  DeclearCallBackEntry(OnDetailsListInfo),
  DeclearCallBackEntry(NULL)
};



void dlgWayPointDetailsShowModal(void){

  TCHAR sTmp[128];
  double sunsettime;
  int sunsethours;
  int sunsetmins;
  WndProperty *wp;

  wf = dlgLoadFromXML(CallBackTable, 
		      LocalPathS(TEXT("dlgWayPointDetails.xml")), 
		      hWndMainWindow,
		      TEXT("IDR_XML_WAYPOINTDETAILS"));

  nTextLines = 0;

  if (!wf) return;

  GetRegistryString(szRegistryWayPointFile, szWaypointFile, MAX_PATH);
  ExpandLocalPath(szWaypointFile);
  ExtractDirectory(Directory, szWaypointFile);

  _stprintf(path_modis,TEXT("%s\\modis-%03d.jpg"),
           Directory,
           SelectedWaypoint+1);

  _stprintf(sTmp, TEXT("%s: "), wf->GetCaption());
  _tcscat(sTmp, WayPointList[SelectedWaypoint].Name);
  wf->SetCaption(sTmp);

  wp = ((WndProperty *)wf->FindByName(TEXT("prpWpComment")));
  wp->SetText(WayPointList[SelectedWaypoint].Comment);
  wp->SetButtonSize(16);

  Units::LongitudeToString(WayPointList[SelectedWaypoint].Longitude, sTmp, sizeof(sTmp)-1);
  ((WndProperty *)wf->FindByName(TEXT("prpLongitude")))
    ->SetText(sTmp);

  Units::LatitudeToString(WayPointList[SelectedWaypoint].Latitude, sTmp, sizeof(sTmp)-1);
  ((WndProperty *)wf->FindByName(TEXT("prpLatitude")))
    ->SetText(sTmp);

  Units::FormatUserAltitude(WayPointList[SelectedWaypoint].Altitude, sTmp, sizeof(sTmp)-1);
  ((WndProperty *)wf->FindByName(TEXT("prpAltitude")))
    ->SetText(sTmp);

  sunsettime = DoSunEphemeris(WayPointList[SelectedWaypoint].Longitude,
                              WayPointList[SelectedWaypoint].Latitude);
  sunsethours = (int)sunsettime;
  sunsetmins = (int)((sunsettime-sunsethours)*60);

  _stprintf(sTmp, TEXT("%02d:%02d"), sunsethours, sunsetmins);
  ((WndProperty *)wf->FindByName(TEXT("prpSunset")))
    ->SetText(sTmp);

  double distance = Distance(GPS_INFO.Latitude,
			 GPS_INFO.Longitude,
			 WayPointList[SelectedWaypoint].Latitude,
			 WayPointList[SelectedWaypoint].Longitude);

  _stprintf(sTmp, TEXT("%.0f %s"), distance*DISTANCEMODIFY,
	    Units::GetDistanceName());
  ((WndProperty *)wf->FindByName(TEXT("prpDistance")))
    ->SetText(sTmp);

  double bearing = Bearing(GPS_INFO.Latitude,
			   GPS_INFO.Longitude,
			   WayPointList[SelectedWaypoint].Latitude,
			   WayPointList[SelectedWaypoint].Longitude);
  _stprintf(sTmp, TEXT("%d�"), iround(bearing));

  ((WndProperty *)wf->FindByName(TEXT("prpBearing")))
    ->SetText(sTmp);

  double alt=0;

  // alt reqd at mc 0
  
  alt = CALCULATED_INFO.NavAltitude - 
    GlidePolar::MacCreadyAltitude(0.0,
				  distance,
				  bearing, 
				  CALCULATED_INFO.WindSpeed, 
				  CALCULATED_INFO.WindBearing, 
				  0, 0, true,
				  0)-SAFETYALTITUDEARRIVAL-
    WayPointList[SelectedWaypoint].Altitude;

  _stprintf(sTmp, TEXT("%.0f %s"), alt*ALTITUDEMODIFY,
	    Units::GetAltitudeName());

  wp = ((WndProperty *)wf->FindByName(TEXT("prpMc0")));
  if (wp) wp->SetText(sTmp);

  // alt reqd at safety mc

  alt = CALCULATED_INFO.NavAltitude - 
    GlidePolar::MacCreadyAltitude(GlidePolar::AbortSafetyMacCready(),
				  distance,
				  bearing, 
				  CALCULATED_INFO.WindSpeed, 
				  CALCULATED_INFO.WindBearing, 
				  0, 0, true,
				  0)-SAFETYALTITUDEARRIVAL-
    WayPointList[SelectedWaypoint].Altitude;

  wp = ((WndProperty *)wf->FindByName(TEXT("prpMc1")));
  if (wp) wp->SetText(sTmp);

  // alt reqd at current mc

  alt = CALCULATED_INFO.NavAltitude - 
    GlidePolar::MacCreadyAltitude(MACCREADY,
				  distance,
				  bearing, 
				  CALCULATED_INFO.WindSpeed, 
				  CALCULATED_INFO.WindBearing, 
				  0, 0, true,
				  0)-SAFETYALTITUDEARRIVAL-
    WayPointList[SelectedWaypoint].Altitude;

  _stprintf(sTmp, TEXT("%.0f %s"), alt*ALTITUDEMODIFY,
	    Units::GetAltitudeName());

  wp = ((WndProperty *)wf->FindByName(TEXT("prpMc2")));
  if (wp) wp->SetText(sTmp);

  /////

  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->FindByName(TEXT("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wInfo    = ((WndFrame *)wf->FindByName(TEXT("frmInfos")));
  wCommand = ((WndFrame *)wf->FindByName(TEXT("frmCommands")));
  wImage   = ((WndOwnerDrawFrame *)wf->FindByName(TEXT("frmImage")));
  wDetails = (WndListFrame*)wf->FindByName(TEXT("frmDetails"));

  ASSERT(wInfo!=NULL);
  ASSERT(wCommand!=NULL);
  ASSERT(wImage!=NULL);
  ASSERT(wDetails!=NULL);

  wDetailsEntry = 
    (WndOwnerDrawFrame*)wf->FindByName(TEXT("frmDetailsEntry"));
  ASSERT(wDetailsEntry!=NULL);
  wDetailsEntry->SetCanFocus(true);

  nTextLines = TextToLineOffsets(WayPointList[SelectedWaypoint].Details,
				 LineOffsets,
				 MAXLINES);

  /* TODO
  wp = ((WndProperty *)wf->FindByName(TEXT("prpWpDetails")));
  wp->SetText(WayPointList[SelectedWaypoint].Details);
  */

  wInfo->SetBorderKind(BORDERLEFT);
  wCommand->SetBorderKind(BORDERLEFT);
  wImage->SetBorderKind(BORDERLEFT | BORDERTOP | BORDERBOTTOM | BORDERRIGHT);
  wDetails->SetBorderKind(BORDERLEFT);

  wCommand->SetVisible(false);
  wImage->SetCaption(TEXT("Blank!"));
  wImage->SetOnPaintNotify(OnImagePaint);

  WndButton *wb;

  wb = ((WndButton *)wf->FindByName(TEXT("cmdGoto")));
  if (wb) 
    wb->SetOnClickNotify(OnGotoClicked);

  wb = ((WndButton *)wf->FindByName(TEXT("cmdReplace")));
  if (wb)
    wb->SetOnClickNotify(OnReplaceClicked);

  wb = ((WndButton *)wf->FindByName(TEXT("cmdNewHome")));
  if (wb) 
    wb->SetOnClickNotify(OnNewHomeClicked);

  wb = ((WndButton *)wf->FindByName(TEXT("cmdTeamCode")));
  if (wb) 
    wb->SetOnClickNotify(OnTeamCodeClicked);

  wb = ((WndButton *)wf->FindByName(TEXT("cmdInserInTask")));
  if (wb) 
    wb->SetOnClickNotify(OnInserInTaskClicked);

  wb = ((WndButton *)wf->FindByName(TEXT("cmdAppendInTask")));
  if (wb) 
    wb->SetOnClickNotify(OnAppendInTaskClicked);

  wb = ((WndButton *)wf->FindByName(TEXT("cmdRemoveFromTask")));
  if (wb) 
    wb->SetOnClickNotify(OnRemoveFromTaskClicked);

  hasimage1 = jpgimage1.Load(wImage->GetDeviceContext() ,path_modis );
  hasimage2 = jpgimage2.Load(wImage->GetDeviceContext() ,path_fname2 );

  page = 0;

  NextPage(0); // JMW just to turn proper pages on/off

  wf->ShowModal();

  delete wf;

  wf = NULL;

}
