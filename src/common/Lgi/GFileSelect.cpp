/*hdr
**      FILE:           GFileSelect.cpp
**      AUTHOR:         Matthew Allen
**      DATE:           20/5/2002
**      DESCRIPTION:    Common file/directory selection dialog
**
**      Copyright (C) 1998-2002, Matthew Allen
**              fret@memecode.com
*/

#include <stdio.h>
#include <stdlib.h>

#include "Lgi.h"
#include "GPopup.h"
#include "GToken.h"
#include "LList.h"
#include "GTextLabel.h"
#include "GEdit.h"
#include "GButton.h"
#include "GCheckBox.h"
#include "GCombo.h"
#include "GTree.h"
#include "GTableLayout.h"
#include "GBox.h"

#define FSI_FILE			0
#define FSI_DIRECTORY		1
#define FSI_BACK			2
#define FSI_UPDIR			3
#define FSI_NEWDIR			4
#define FSI_DESKTOP			5
#define FSI_HARDDISK		6
#define FSI_CDROM			7
#define FSI_FLOPPY			8
#define FSI_NETWORK			9

enum DlgType
{
	TypeNone,
	TypeOpenFile,
	TypeOpenFolder,
	TypeSaveFile
};

char ModuleName[] = "File Select";

uint32 IconBits[] = {
0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0x0000F81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xC980FA8A, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0x738E738E, 0xF81F738E, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F,
0xF81FF81F, 0x8430F81F, 0x84308430, 0x84308430, 0xF81F8430, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0x00000000, 0x00000000, 0x00000000, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0x9CE09CE0, 0x9CE09CE0, 0x00009CE0, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xCCE0F81F, 0x9800FCF9, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F,
0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0x738E738E, 0xCE6C9E73, 0x738EC638, 0xF81F738E, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0x9FFF738E, 0x9FFF9FFF, 0x9FFF9FFF, 0x00009FFF, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xFFFF0000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0x9CE0F81F, 0xFFF9F7BE, 0xFFF3FFF9, 0x9CE0FFF3, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0x9CE09CE0, 0x9CE09CE0, 0x00009CE0, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0x0000F81F, 0xF81FF81F, 0xF81FF81F, 0x04F904F9,
0x04F904F9, 0xAD720313, 0xAD72AD72, 0xAD72AD72, 0xFE60CCE0, 0x00009B00, 0x0000AD72, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0x738EF81F, 0x667334F3, 0xCE6C9E73, 0xC638B5B6, 0x3186C638, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xFFFF738E, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xF81FF81F, 0x738E738E, 0x738E738E, 0x738E738E, 0x738E738E, 0x738E738E, 0x738E738E, 0xF81F738E, 0xFFFF0000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xCE6C9CE0, 0xCE6CCE6C, 0xCE6CCE6C, 0xCE6CCE6C, 0x9CE09CE0, 0x9CE09CE0, 0x9CE09CE0, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0x0000F81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0x9CE0F81F, 0xFFF9F7BE, 0xFFF3FFF9, 0x9CE0FFF3,
0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xF81F0000, 0x667F04F9, 0x031304F9, 0xFFFFCE73, 0xFFF9FFFF, 0xCCE0FFFF, 0x9B00FFF3, 0x04F90000, 0x00000313, 0xF81FF81F, 0x738E738E, 0x738E738E, 0x738E738E, 0x738E738E, 0x738E738E, 0x738E738E, 0xF81F738E, 0xF81FF81F, 0xF81FF81F, 0x6673738E, 0x34F36673, 0xCE736673, 0xB5B6C638, 0xB5B6DEFB, 0xF81F3186, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xFFFF738E, 0xFFFFFFFF, 0xCFFFCFFF, 0x0000CFFF, 0x738EF81F, 0xB5B6B5B6, 0xB5B6B5B6, 0xB5B6B5B6, 0xB5B6B5B6, 0xB5B6B5B6, 0xB5B6B5B6, 0x0000738E, 0xFFFF0000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0x0000FFFF, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xFFF99CE0, 0xFFF9FFF9, 0xFFF9FFF9, 0xFFF9FFF9, 0xFFF9FFF9, 0xFFF9FFF9, 0xCE6CFFF3,
0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0x00000000, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xCE6C9CE0, 0xCE6CCE6C, 0xCE6CCE6C, 0xCE6CCE6C, 0x9CE09CE0, 0x9CE09CE0, 0x9CE09CE0, 0xF81FF81F, 0xF81FF81F, 0x9CE09CE0, 0x9CE09CE0, 0xF81F0000, 0x0000F81F, 0x0000F81F, 0x0000F81F, 0xF81FF81F, 0x04F904F9, 0xCE730313, 0xFFFFFFFF, 0xFFF9FFF9, 0xFE60CCE0, 0x00009B00, 0x667F3313, 0x00000313, 0x738EF81F, 0xB5B6B5B6, 0xB5B6B5B6, 0xB5B6B5B6, 0xB5B6B5B6, 0xB5B6B5B6, 0xB5B6B5B6, 0x0000738E, 0xF81FF81F, 0xF81FF81F, 0x34F98430, 0x667364F9, 0x84306679, 0xD6BAB5B6, 0xB5B6C638, 0xF81F3186, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xCFFF738E, 0xCFFFCFFF, 0xCFFFCFFF, 0x0000CFFF, 0xFFFF738E, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x9CF3FFFF, 0x0000738E, 0xFFFF0000, 0xFFFFFFFF,
0xFFFFFFFF, 0x00000000, 0x00000000, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xFFF99CE0, 0xFFF3FFF3, 0xFFF3FFF3, 0xFFF3FFF3, 0xFFF3FFF3, 0xFFF3FFF3, 0xCE6CFE73, 0xF81F0000, 0xF81FF81F, 0x0000F81F, 0x000007FF, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xFFF99CE0, 0xFFF9FFF9, 0xFFF9FFF9, 0xFFF9FFF9, 0xFFF9FFF9, 0xFFF9FFF9, 0xCE6CFFF3, 0xF81F0000, 0x9CE0F81F, 0xFFF9F7BE, 0xFFF3FFF3, 0x00009CE0, 0xF81FF81F, 0xF81F0000, 0xF81F0000, 0xF81FF81F, 0x031304F9, 0xFFFFCE73, 0x94B294B2, 0xCCE094B2, 0x9B00FFF3, 0xAD720000, 0x3313FFF9, 0x00000313, 0xFFFF738E, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x9CF3FFFF, 0x0000738E, 0xF81FF81F, 0x738EF81F, 0xA53494B2, 0x667964F3, 0x00008430, 0xA5348430, 0xCE79B5B6, 0x3186CE79, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xCE79738E,
0xCE79C638, 0xC638B5B6, 0x0000B5B6, 0xD6BA738E, 0xC638C638, 0xC638C638, 0xC638C638, 0xB5B6C638, 0x04200660, 0x94B2B5B6, 0x0000738E, 0xFFFF0000, 0xFFFFFFFF, 0xFFFFFFFF, 0xD699FFFF, 0xD699D699, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xFFF99CE0, 0xFFF3FFF3, 0xFFF3FFF3, 0xFFF3FFF3, 0xFE73FFF3, 0xFE73FFF3, 0xCE6CFFF3, 0xF81F0000, 0xF81FF81F, 0x07FF0000, 0x000007FF, 0x00000000, 0x00000000, 0x00000000, 0xF81F0000, 0xF81FF81F, 0xFFF99CE0, 0xFFF3FFF3, 0x0000FFF3, 0xFFF3FFF3, 0xFFF3FFF3, 0xFFF3FFF3, 0xCE6CFE73, 0xF81F0000, 0xCE6C9CE0, 0xCE6CCE6C, 0xCE6CCE6C, 0x9CE0CE6C, 0x9CE09CE0, 0xF81F9CE0, 0x0000F81F, 0x0000F81F, 0xCE730313, 0xFFFFFFFF, 0xFFFF94B2, 0xFE60CCE0, 0x00009B00, 0xFFF9AD72, 0xCE73CE73, 0x00003313, 0xD6BA738E, 0xC638C638, 0xC638C638, 0xC638C638, 0xB5B6C638, 0x04200660, 0x94B2B5B6, 0x0000738E,
0xF81FF81F, 0x738EF81F, 0xB5B6B5B6, 0x8430CE79, 0xF81F0000, 0x84300000, 0xCE79CE79, 0x3186CE79, 0xF81FF81F, 0x84308430, 0x84308430, 0x84308430, 0xC638738E, 0x84308430, 0x84308430, 0x0000C638, 0xDEFB738E, 0xC638B5B6, 0xC638C638, 0xC638C638, 0xB5B6B5B6, 0xB5B6B5B6, 0x94B2B5B6, 0x0000738E, 0xFFFF0000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xD699FFFF, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xFFF99CE0, 0xFFF3FFF3, 0xFFF3FFF3, 0xFFF3FFF3, 0xFFF3FFF3, 0xFFF3FE73, 0xCE6CFE73, 0xF81F0000, 0x0000F81F, 0x07FF07FF, 0x07FF07FF, 0x07FF07FF, 0x07FF07FF, 0x07FF07FF, 0xF81F0000, 0xF81FF81F, 0xFFF99CE0, 0xFFF3FFF3, 0x00000000, 0xFFF30000, 0xFE73FFF3, 0xFE73FFF3, 0xCE6CFFF3, 0xF81F0000, 0xFFF99CE0, 0xFFF9FFF9, 0xFFF9FFF9, 0xFFF9FFF9, 0xFFF3FFF9, 0x0000CE6C, 0xF81F0000, 0xF81FF81F, 0xFFFFAD72, 0xFFF9FFFF, 0xFFFF94B2,
0x9B009CEC, 0x00000000, 0xCE73FFF9, 0xAD72FFF9, 0x000094B2, 0xDEFB738E, 0xC638B5B6, 0xC638C638, 0xC638C638, 0xB5B6B5B6, 0xB5B6B5B6, 0x94B2B5B6, 0x0000738E, 0xF81FF81F, 0x738EF81F, 0xF7BEE73C, 0xB5B6E73C, 0x00008430, 0xB5B68430, 0xF7BEEF7D, 0x3186CE79, 0x8430F81F, 0xC638C638, 0xC638C638, 0xC638C638, 0xCE79738E, 0x0000738E, 0xFFFF738E, 0x0000B5B6, 0xDEFB738E, 0x738E738E, 0x738E738E, 0x738E738E, 0x738E738E, 0x738E738E, 0x94B2B5B6, 0x0000738E, 0xFFFF0000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xD699FFFF, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xFFF99CE0, 0xFFF3FFF3, 0xFFF3FFF3, 0xFE73FFF3, 0xFE73FFF3, 0xFE73FFF3, 0xCE6CFFF3, 0xF81F0000, 0x0000F81F, 0x07FF07FF, 0x07FF07FF, 0x07FF07FF, 0x07FF07FF, 0x07FF07FF, 0xF81F0000, 0xF81FF81F, 0xFFF99CE0, 0x0000FFF3, 0x00000000, 0x00000000, 0xFFF3FFF3, 0xFFF3FE73,
0xCE6CFE73, 0xF81F0000, 0xFFF99CE0, 0xFFF3FFF3, 0xFFF3FFF3, 0xFFF3FFF3, 0xFE73FFF3, 0x0000CE6C, 0x0000F81F, 0xF81FF81F, 0xFFFFAD72, 0xFFF9FFF9, 0xFFFF94B2, 0xFFFF0000, 0x0000FFF9, 0xFFF9CE73, 0xCE73AD72, 0x000094B2, 0xDEFB738E, 0x738E738E, 0x738E738E, 0x738E738E, 0x738E738E, 0x738E738E, 0x94B2B5B6, 0x0000738E, 0x8430F81F, 0x84308430, 0xA5348430, 0xA534B5B6, 0x8430A534, 0xDEFB34F3, 0xFFFFE73C, 0xF81F3186, 0xFFFF8430, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x0000FFFF, 0x00000000, 0x00000000, 0xF81F0000, 0xD6BA738E, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x94B2B5B6, 0x0000738E, 0xFFFF0000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xD699FFFF, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xFFF99CE0, 0xFFF3FFF3, 0xFFF3FFF3, 0xFFF3FFF3, 0xFFF3FE73, 0xFFF3FE73, 0xCE6CFE73, 0xF81F0000, 0xF81FF81F,
0x07FF0000, 0x000007FF, 0x00000000, 0x00000000, 0x00000000, 0xF81F0000, 0xF81FF81F, 0xFFF99CE0, 0xFFF3FFF3, 0x0000FFF3, 0xFE73FFF3, 0xFE73FFF3, 0xFE73FFF3, 0xCE6CFFF3, 0xF81F0000, 0xFFF99CE0, 0xFFF3FFF3, 0xFFF3FFF3, 0xFFF3FFF3, 0xFFF3FE73, 0x0000CE6C, 0xF81FF81F, 0xF81F0000, 0xFFF9AD72, 0xFFF9FFF9, 0xFFFF94B2, 0xFFFFFFFF, 0x0000FFF9, 0xCE73FFF9, 0xAD72CE73, 0x000094B2, 0xD6BA738E, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x94B2B5B6, 0x0000738E, 0xFFFF8430, 0xFFFFFFFF, 0xA534738E, 0xB5B6A534, 0xCE73D6BA, 0x34F96673, 0xB5B6CFFF, 0xF81F3186, 0xC6388430, 0xC638C638, 0xC638C638, 0xC638C638, 0xC638C638, 0xC638F800, 0x738E8430, 0xF81F0000, 0x738E738E, 0x738E738E, 0x738E738E, 0x738E738E, 0x738E738E, 0x738E738E, 0x738E738E, 0xF81F0000, 0xFFFF0000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
0xD699FFFF, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xFFF99CE0, 0xFFF3FFF3, 0xFE73FFF3, 0xFE73FFF3, 0xFE73FFF3, 0xFE73FFF3, 0xCE6CFE73, 0xF81F0000, 0xF81FF81F, 0x0000F81F, 0x000007FF, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xFFF99CE0, 0xFFF3FFF3, 0x0000FFF3, 0xFFF3FFF3, 0xFFF3FE73, 0xFFF3FE73, 0xCE6CFE73, 0xF81F0000, 0xFFF99CE0, 0xFFF3FFF3, 0xFFF3FFF3, 0xFE73FFF3, 0xFE73FFF3, 0x0000CE6C, 0xF81FF81F, 0xF81FF81F, 0xFFF9AD72, 0xFFF9FFF9, 0xFFFF94B2, 0xFFFFFFFF, 0x0000FFF9, 0xCE73CE73, 0xCE73AD72, 0x000094B2, 0x738E738E, 0x738E738E, 0x738E738E, 0x738E738E, 0x738E738E, 0x738E738E, 0x738E738E, 0xF81F0000, 0xFFFF8430, 0xC638C638, 0x738EC638, 0xB5B6A534, 0xCE73CE79, 0x04F99E73, 0x000004F9, 0xF81FF81F, 0xC6388430, 0xC638C638, 0x84308430, 0x84308430, 0xC638C638, 0xC638C638, 0x738E8430,
0xF81F0000, 0x0000F81F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xF81FF81F, 0xFFFF0000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xD699FFFF, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xFFF99CE0, 0xFFF3FE73, 0xFFF3FE73, 0xFFF3FE73, 0xFFF3FE73, 0xFE73FE73, 0xCE6CFE73, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0x00000000, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xFFF99CE0, 0xFFF3FFF3, 0x0000FFF3, 0x00000000, 0x00000000, 0xFE730000, 0xCE6CFE73, 0xF81F0000, 0xFFF99CE0, 0xFFF3FFF3, 0xFFF3FE73, 0xFFF3FE73, 0xFFF3FE73, 0x0000CE6C, 0xF81FF81F, 0xF81FF81F, 0xFFF904F9, 0xFFF9FFF9, 0xFFF994B2, 0xFFF9FFF9, 0x0000FFF9, 0xAD72CE73, 0xAD72CE73, 0x00003313, 0x0000F81F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xF81FF81F, 0xFFFF8430, 0x31863186,
0x31863186, 0x84308430, 0xCE73CE79, 0x31869E73, 0x00003186, 0xF81FF81F, 0xC6388430, 0x84308430, 0x00000000, 0x00000000, 0x84308430, 0xC6388430, 0x738E8430, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xB5B6F81F, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xFFFF0000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xD699FFFF, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xCE6C9CE0, 0xCE6CCE6C, 0xCE6CCE6C, 0xCE6CCE6C, 0xCE6CCE6C, 0xCE6CCE6C, 0xCE6CCE6C, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0x0000F81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xFFF99CE0, 0xFFF3FE73, 0xFFF3FE73, 0xFFF3FE73, 0xFFF3FE73, 0xFE73FE73, 0xCE6CFE73, 0xF81F0000, 0xFFF99CE0, 0xFFF3FE73, 0xFE73FFF3, 0xFE73FFF3, 0xFE73FFF3, 0x0000CE6C, 0xF81FF81F, 0xF81FF81F, 0x04F904F9, 0xFFF9FFF9, 0x00000000, 0x00000000, 0x00000000,
0xCE73AD72, 0x3313AD72, 0x00003313, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xFFFF8430, 0x042007E0, 0xFFFFFFFF, 0xC638C638, 0x31863186, 0xC6383186, 0x0000738E, 0xF81FF81F, 0xC6388430, 0xC638C638, 0xFFFFFFFF, 0xFFFFFFFF, 0xC638C638, 0xC638C638, 0x738E8430, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xB5B6F81F, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xFFFF0000, 0xD699D699, 0xD699D699, 0xD699D699, 0xD699D699, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0x0000F81F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xCE6C9CE0, 0xCE6CCE6C, 0xCE6CCE6C, 0xCE6CCE6C, 0xCE6CCE6C, 0xCE6CCE6C, 0xCE6CCE6C, 0xF81F0000,
0xCE6C9CE0, 0xCE6CCE6C, 0xCE6CCE6C, 0xCE6CCE6C, 0xCE6CCE6C, 0x0000CE6C, 0xF81FF81F, 0xF81FF81F, 0x667F04F9, 0xFFF904F9, 0xCE73FFF9, 0xCE73FFF9, 0xAD72CE73, 0xAD72CE73, 0x667F3313, 0x00003313, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0x8430738E, 0x84308430, 0x84308430, 0x84308430, 0x84308430, 0x84308430, 0x00008430, 0xF81FF81F, 0x84308430, 0x84308430, 0x84308430, 0x84308430, 0x84308430, 0x84308430, 0x00008430, 0xF81FF81F, 0xB5B6F81F, 0xB5B6F81F, 0xB5B6B5B6, 0xB5B6B5B6, 0xB5B6B5B6, 0xB5B6B5B6, 0xF81FB5B6, 0xF81FB5B6, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F,
0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0x0000F81F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xF81F0000, 0x0000F81F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xF81FF81F, 0xF81FF81F, 0x03130313, 0x04F90313, 0x94B294B2, 0x94B294B2, 0x94B294B2, 0x331394B2, 0x33133313, 0x00003313, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0x0000F81F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xF81F0000, 0xF81FF81F, 0x0000F81F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xF81F0000, 0xF81FF81F, 0x0000F81F, 0x0000F81F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xF81F0000, 0xF81F0000, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F,
0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F,
0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F, 0xF81FF81F};

GInlineBmp FileSelectIcons = { 160, 16, 16, IconBits };

//////////////////////////////////////////////////////////////////////////
char *GFileType::DefaultExtension()
{
	char *Status = 0;
	GToken T(Extension(), ";");
	if (T.Length())
	{
		char s[256];
		strcpy(s, T[0]);
		char *Dir = strchr(s, '.');
		if (Dir)
		{
			Status = NewStr(Dir+1);
			if (Status) strlwr(Status);
		}
	}
	return Status;
}

//////////////////////////////////////////////////////////////////////////
class GFolderItem : public LListItem
{
	class GFileSelectDlg *Dlg;
	char *Path;

public:
	char *File;
	bool IsDir;

	GFolderItem(GFileSelectDlg *dlg, char *FullPath, GDirectory *Dir);
	~GFolderItem();


	void OnActivate();
	char *GetText(int i);
	int GetImage(int Flags);
	void OnSelect();
	void OnDelete(bool Ask = true);
	void OnRename();
	void OnMouseClick(GMouse &m);
};


//////////////////////////////////////////////////////////////////////////
// This is just a private data container to make it easier to change the
// implementation of this class without effecting headers and applications.
class GFileSelectPrivate
{
	friend class GFileSelect;
	friend class GFileSelectDlg;
	friend class GFolderList;

	GView *Parent;
	GFileSelect *Select;

	DlgType Type;
	char *Title;
	char *DefExt;
	bool MultiSelect;
	List<char> Files;
	int CurrentType;
	List<GFileType> Types;
	List<char> History;
	bool ShowReadOnly;
	bool ReadOnly;
	
	bool EatClose;

public:
	static GImageList *Icons;
	static char *InitPath;
	static bool InitShowHiddenFiles;
	static GRect InitSize;

	GFileSelectPrivate(GFileSelect *select)
	{
		ShowReadOnly = false;
		ReadOnly = false;
		EatClose = false;
		Select = select;
		Type = TypeNone;
		Title = 0;
		DefExt = 0;
		MultiSelect = false;
		Parent = 0;
		CurrentType = -1;

		if (!Icons)
			Icons = new GImageList(16, 16, FileSelectIcons.Create(0xF81F));
	}

	virtual ~GFileSelectPrivate()
	{
		DeleteArray(Title);
		DeleteArray(DefExt);

		Types.DeleteObjects();
		Files.DeleteArrays();
		History.DeleteArrays();
	}
};

GImageList *GFileSelectPrivate::Icons = 0;
char *GFileSelectPrivate::InitPath = 0;
bool GFileSelectPrivate::InitShowHiddenFiles = false;
GRect GFileSelectPrivate::InitSize(0, 0, 600, 500);

//////////////////////////////////////////////////////////////////////////
// This class implements the UI for the selector.
class GFileSelectDlg;

class GFolderView
{
protected:
	GFileSelectDlg *Dlg;

public:
	GFolderView(GFileSelectDlg *dlg)
	{
		Dlg = dlg;
	}

	virtual void OnFolder() {}
};

class GFolderDrop : public GDropDown, public GFolderView
{
public:
	GFolderDrop(GFileSelectDlg *dlg, int Id, int x, int y, int cx, int cy);

	void OnFolder();

	bool OnLayout(GViewLayoutInfo &Inf)
	{
		Inf.Width.Min = 
			Inf.Width.Max = 18;
		return true;
	}
};

class GIconButton : public GLayout
{
	GImageList *Icons;
	int Icon;
	bool Down;

public:
	GIconButton(int Id, int x, int y, int cx, int cy, GImageList *icons, int icon)
	{
		Icons = icons;
		Icon = icon;
		SetId(Id);
		GRect r(x, y, x+cx, y+cy);
		SetPos(r);
		Down = false;
		SetTabStop(true);
	}

	void OnPaint(GSurface *pDC)
	{
		GRect c = GetClient();
		GColour Background(LC_MED, 24);
		c.Offset(-c.x1, -c.y1);
		LgiWideBorder(pDC, c, Down ? DefaultSunkenEdge : DefaultRaisedEdge);
		pDC->Colour(Background);
		pDC->Rectangle(&c);
		
		int x = (c.X()-Icons->TileX()) / 2;
		int y = (c.Y()-Icons->TileY()) / 2;

		if (Focus())
		{
			#if WINNATIVE
			RECT r = c;
			DrawFocusRect(pDC->Handle(), &r);
			#endif
		}

		Icons->Draw(pDC, c.x1+x+Down, c.y1+y+Down, Icon, Background, !Enabled());
	}

	void OnFocus(bool f)
	{
		Invalidate();
	}

	void OnMouseClick(GMouse &m)
	{
		if (Enabled())
		{
			bool Trigger = Down && !m.Down();

			Capture(Down = m.Down());
			if (Down) Focus(true);
			Invalidate();		

			if (Trigger)
			{
				GViewI *n=GetNotify()?GetNotify():GetParent();
				if (n) n->OnNotify(this, 0);
			}
		}
	}

	void OnMouseEnter(GMouse &m)
	{
		if (IsCapturing())
		{
			Down = true;
			Invalidate();
		}
	}

	void OnMouseExit(GMouse &m)
	{
		if (IsCapturing())
		{
			Down = false;
			Invalidate();
		}
	}

	bool OnKey(GKey &k)
	{
		if (k.c16 == ' ' || k.c16 == VK_RETURN)
		{
			if (Enabled() && Down ^ k.Down())
			{
				Down = k.Down();
				Invalidate();

				if (!Down)
				{
					GViewI *n=GetNotify()?GetNotify():GetParent();
					if (n) n->OnNotify(this, 0);
				}
			}
			
			return true;
		}
		
		return false;
	}
	
	bool OnLayout(GViewLayoutInfo &Inf)
	{
		Inf.Width.Min = Inf.Width.Max = Icons->TileX() + 4;
		Inf.Width.Max += 4;
		Inf.Height.Min = Inf.Height.Max = Icons->TileY() + 4;
		Inf.Height.Max += 4;
		return true;
	}
};

class GFolderList : public LList, public GFolderView
{
	GString FilterKey;

public:
	GFolderList(GFileSelectDlg *dlg, int Id, int x, int y, int cx, int cy);
	~GFolderList()
	{
	}	

	void OnFolder();
	bool OnKey(GKey &k);
	void SetFilterKey(GString s) { FilterKey = s; OnFolder(); }
};

#define IDC_STATIC					-1
#define IDD_FILE_SELECT				1000
#define IDC_PATH					1002
#define IDC_DROP					1003
#define IDC_BACK					1004
#define IDC_UP						1005
#define IDC_NEW						1006
#define IDC_VIEW					1007
#define IDC_FILE					1010
#define IDC_TYPE					1011
#define IDC_SHOWHIDDEN				1012
#define IDC_SUB_TBL					1013
#define IDC_BOOKMARKS				1014
#define IDC_FILTER					1015
#define IDC_FILTER_CLEAR			1016

#if 1
#define USE_FOLDER_CTRL				1

enum FolderCtrlMessages
{
	M_DELETE_EDIT				=	M_USER + 100,
	M_NOTIFY_VALUE_CHANGED,
};

class FolderCtrl : public GView
{
	struct Part
	{
		GAutoPtr<GDisplayString> ds;
		GRect Arrow;
		GRect Text;
	};
	
	GEdit *e;
	GArray<Part> p;
	Part *Over;
	int Cursor;

	Part *HitPart(int x, int y, int *Sub = NULL)
	{
		for (unsigned i=0; i<p.Length(); i++)
		{
			if (p[i].Arrow.Overlap(x, y))
			{
				if (Sub) *Sub = 1;
				return p.AddressOf(i);
			}
			if (p[i].Text.Overlap(x, y))
			{
				if (Sub) *Sub = 0;
				return p.AddressOf(i);
			}
		}

		return NULL;
	}
	
public:
	FolderCtrl(int id)
	{
		e = NULL;
		Cursor = 0;
		SetId(id);
	}
	
	~FolderCtrl()
	{
	}
	
	const char *GetClass() { return "FolderCtrl"; }
	
	bool OnLayout(GViewLayoutInfo &Inf)
	{
		if (Inf.Width.Min == 0)
		{
			Inf.Width.Min = -1;
			Inf.Width.Max = -1;
		}
		else
		{
			Inf.Height.Min = GetFont()->GetHeight() + 4;
			Inf.Height.Max = Inf.Height.Min;
		}
		return true;
	}

	GString NameAt(int Level)
	{
		GString n;
		#ifndef WINDOWS
		n += "/";
		#endif
		for (unsigned i=0; i<=Level && i<p.Length(); i++)
		{
			n += (const char16*) *(p[i].ds.Get());
			n += DIR_STR;
		}

		return n;		
	}

	char *Name()
	{
		GString n = NameAt(Cursor);
		GBase::Name(n);
		return GBase::Name();
	}
	
	bool Name(const char *n)
	{
		if (n && GView::Name() && !strcmp(n, GView::Name()))
			return true;

		bool b = GView::Name(n);
		
		Over = NULL;
		GString Nm(n);
		GString::Array a = Nm.SplitDelimit(DIR_STR);
		p.Length(0);
		for (size_t i=0; i<a.Length(); i++)
		{
			Part &n = p.New();
			n.ds.Reset(new GDisplayString(GetFont(), a[i]));
		}

		Cursor = p.Length() - 1;
		Invalidate();
		return b;
	}
	
	void OnPaint(GSurface *pDC)
	{
		GRect c = GetClient();
		LgiThinBorder(pDC, c, EdgeWin7Sunken);
		
		GFont *f = GetFont();
		f->Transparent(false);
		
		GDisplayString Arrow(f, ">");
		for (unsigned i=0; i<p.Length(); i++)
		{
			Part *n = p.AddressOf(i);
			COLOUR Fore = Cursor == i ? LC_FOCUS_SEL_FORE : LC_TEXT;
			COLOUR Bk = Cursor == i ? LC_FOCUS_SEL_BACK : (n == Over ? GdcMixColour(LC_FOCUS_SEL_BACK, LC_WORKSPACE, 0.15f) : LC_WORKSPACE);
			
			// Layout and draw arrow
			n->Arrow.ZOff(Arrow.X()+1, c.Y()-1);
			n->Arrow.Offset(c.x1, c.y1);
			f->Colour(Rgb24(192,192,192), Bk);
			Arrow.DrawCenter(pDC, &n->Arrow);
			c.x1 = n->Arrow.x2 + 1;

			if (n->ds)
			{
				// Layout and draw text
				n->Text.ZOff(n->ds->X() + 4, c.Y()-1);
				n->Text.Offset(c.x1, c.y1);
				f->Colour(Fore, Bk);
				n->ds->DrawCenter(pDC, &n->Text);
				c.x1 = n->Text.x2 + 1;
			}
		}
		
		pDC->Colour(LC_WORKSPACE, 24);
		pDC->Rectangle(&c);
	}

	void OnMouseClick(GMouse &m)
	{
		if (m.IsContextMenu())
		{
		}
		else if (m.Left())
		{
			if (m.Down())
			{
				if (p.PtrCheck(Over))
				{
					// Over a path node...
					Cursor = Over - p.AddressOf(0);
					Part &o = p[Cursor];
					Invalidate();
					SendNotify(GNotifyValueChanged);

					if (o.Arrow.Overlap(m.x, m.y))
					{
						// Show sub-menu at this level
						ShowMenu(Cursor);
					}
				}
				else
				{
					// In empty space
					if (!e)
					{
						GRect c = GetClient();
						e = new GEdit(GetId()+1, c.x1, c.y1, c.X()-1, c.Y()-1);
						if (e)
						{
							e->Attach(this);

							GString s = Name();
							e->Name(s);
							e->SetCaret(s.Length());
							e->Focus(true);
						}
					}
				}
			}
		}
	}

	void OnMouseMove(GMouse &m)
	{
		Part *o = Over;
		Over = HitPart(m.x, m.y);
		if (o != Over)
			Invalidate();
	}
	
	void OnMouseExit(GMouse &m)
	{
		if (Over)
		{
			Over = NULL;
			Invalidate();
		}
	}

	int OnNotify(GViewI *c, int f)
	{
		if (e != NULL &&
			c->GetId() == e->GetId())
		{
			if (f == VK_RETURN)
			{
				GString s = e->Name();
				Name(s);
				PostEvent(M_DELETE_EDIT);
				PostEvent(M_NOTIFY_VALUE_CHANGED);
			}
		}

		return 0;
	}

	GMessage::Result OnEvent(GMessage *m)
	{
		switch (m->Msg())
		{
			case M_DELETE_EDIT:
			{
				DeleteObj(e);
				break;
			}
			case M_NOTIFY_VALUE_CHANGED:
			{
				SendNotify(GNotifyValueChanged);
				break;			
			}
		}

		return GView::OnEvent(m);
	}

	virtual bool ShowMenu(int Level)
	{
		if (Level <= 0)
			return false;

		GString dir = NameAt(Level-1);
		GSubMenu s;
		GDirectory d;

		GString::Array Opts;
		for (int b = d.First(dir); b; b = d.Next())
		{
			if (d.IsDir())
			{
				Opts.New() = d.GetName();
				s.AppendItem(d.GetName(), Opts.Length());
			}
		}

		Part &i = p[Level];
		GdcPt2 pt(i.Arrow.x1, i.Arrow.y2+1);
		PointToScreen(pt);
		int Cmd = s.Float(this, pt.x, pt.y, true);
		if (Cmd)
		{
			GString np;
			np = dir + DIR_STR + Opts[Cmd-1];
			Name(np);
			PostEvent(M_NOTIFY_VALUE_CHANGED);
		}
		else return false;

		return true;
	}
};

#else
#define USE_FOLDER_CTRL				0
#endif


class GFileSelectDlg :
	public GDialog
{
	GRect OldPos;
	GRect MinSize;
	GArray<GString> Links;
	GArray<GFolderItem*> Hidden;
	
public:
	GFileSelectPrivate *d;

	GTableLayout *Tbl;
	GBox *Sub;

	GTree *Bookmarks;
	GTextLabel *Ctrl1;

	#if USE_FOLDER_CTRL
	FolderCtrl *Ctrl2;
	#else
	GEdit *Ctrl2;
	#endif
	
	GFolderDrop *Ctrl3;
	GIconButton *BackBtn;
	GIconButton *UpBtn;
	GIconButton *NewDirBtn;
	GFolderList *FileLst;
	GTextLabel *Ctrl8;
	GTextLabel *Ctrl9;
	GEdit *FileNameEdit;
	GCombo *FileTypeCbo;
	GButton *SaveBtn;
	GButton *CancelBtn;
	GCheckBox *ShowHidden;
	GEdit *FilterEdit;

	GFileSelectDlg(GFileSelectPrivate *Select);
	~GFileSelectDlg();

	int OnNotify(GViewI *Ctrl, int Flags);
	void OnUpFolder();
	void SetFolder(char *f);
	void OnFolder();
	void OnFile(char *f);
	void OnFilter(const char *Key);

	bool OnViewKey(GView *v, GKey &k)
	{
		if (k.vkey == VK_UP && k.Alt())
		{
			if (k.Down())
			{
				UpBtn->SendNotify();
			}
			
			return true;
		}

		return false;
	}
	
	void Add(GTreeItem *i, GVolume *v)
	{
		if (!i || !v)
			return;
		i->SetText(v->Name(), 0);
		i->SetText(v->Path(), 1);
		
		for (unsigned n=0; n<Links.Length(); n++)
		{
			if (v->Path() && !_stricmp(v->Path(), Links[n]))
			{
				Links.DeleteAt(n--);
				break;
			}
		}
		
		for (GVolume *cv = v->First(); cv; cv = cv->Next())
		{
			GTreeItem *ci = new GTreeItem;
			if (ci)
			{
				i->Insert(ci);
				i->Expanded(true);
				Add(ci, cv);
			}
		}
	}
};

GFileSelectDlg::GFileSelectDlg(GFileSelectPrivate *select)
{
	SaveBtn = 0;
	BackBtn = 0;
	CancelBtn = 0;
	ShowHidden = 0;
	FileTypeCbo = 0;
	FileNameEdit = 0;
	Ctrl8 = 0;
	Ctrl9 = 0;
	FileLst = 0;
	Ctrl3 = 0;
	BackBtn = 0;
	UpBtn = 0;
	NewDirBtn = 0;
	Ctrl2 = 0;
	Sub = NULL;
	Bookmarks = NULL;
	FilterEdit = NULL;

	d = select;
	SetParent(d->Parent);
	MinSize.ZOff(450, 300);
	OldPos.Set(0, 0, 475, 350 + LgiApp->GetMetric(LGI_MET_DECOR_Y) );
	SetPos(OldPos);

	int x = 0, y = 0;
	AddView(Tbl = new GTableLayout);

	// Top Row
	GLayoutCell *c = Tbl->GetCell(x++, y);
	c->Add(Ctrl1 = new GTextLabel(IDC_STATIC, 0, 0, -1, -1, "Look in:"));
	c->VerticalAlign(GCss::Len(GCss::VerticalMiddle));
	c = Tbl->GetCell(x++, y);
	#if USE_FOLDER_CTRL
	c->Add(Ctrl2 = new FolderCtrl(IDC_PATH));
	#else
	c->Add(Ctrl2 = new GEdit(IDC_PATH, 0, 0, 245, 21, ""));
	#endif
	c = Tbl->GetCell(x++, y);
	c->Add(Ctrl3 = new GFolderDrop(this, IDC_DROP, 336, 7, 16, 21));
	c = Tbl->GetCell(x++, y);
	c->Add(BackBtn = new GIconButton(IDC_BACK, 378, 7, 27, 21, d->Icons, FSI_BACK));
	c = Tbl->GetCell(x++, y);
	c->Add(UpBtn = new GIconButton(IDC_UP, 406, 7, 27, 21, d->Icons, FSI_UPDIR));
	c = Tbl->GetCell(x++, y);
	c->Add(NewDirBtn = new GIconButton(IDC_NEW, 434, 7, 27, 21, d->Icons, FSI_NEWDIR));

	// Folders/items row
	x = 0; y++;
	c = Tbl->GetCell(x, y, true, 6, 1);
	c->Add(Sub = new GBox(IDC_SUB_TBL));
	Sub->AddView(Bookmarks = new GTree(IDC_BOOKMARKS, 0, 0, -1, -1));
	Bookmarks->GetCss(true)->Width(GCss::Len(GCss::LenPx, 150.0));
	
	GTableLayout *t;
	Sub->AddView(t = new GTableLayout(11));
	
	// Filter / search row
	c = t->GetCell(0, 0);
	c->Add(new GCheckBox(IDC_FILTER_CLEAR, 0, 0, -1, -1, "Filter items:"));
	c->VerticalAlign(GCss::Len(GCss::VerticalMiddle));
	c = t->GetCell(1, 0);
	c->Add(FilterEdit = new GEdit(IDC_FILTER, 0, 0, 60, 20));
	c = t->GetCell(0, 1, true, 2);
	c->Add(FileLst = new GFolderList(this, IDC_VIEW, 14, 35, 448, 226));
	
	// File name row
	x = 0; y++;
	c = Tbl->GetCell(x++, y);
	c->Add(Ctrl8 = new GTextLabel(IDC_STATIC, 14, 275, -1, -1, "File name:"));
	c = Tbl->GetCell(x, y, true, 2);
	x += 2;
	c->Add(FileNameEdit = new GEdit(IDC_FILE, 100, 268, 266, 21, ""));
	c = Tbl->GetCell(x, y, true, 3);
	c->Add(SaveBtn = new GButton(IDOK, 392, 268, 70, 21, "Ok"));

	// 4th row
	x = 0; y++;
	c = Tbl->GetCell(x++, y);
	c->Add(Ctrl9 = new GTextLabel(IDC_STATIC, 14, 303, -1, -1, "Files of type:"));
	c = Tbl->GetCell(x, y, true, 2);
	x += 2;
	c->Add(FileTypeCbo = new GCombo(IDC_TYPE, 100, 296, 266, 21, ""));
	c = Tbl->GetCell(x++, y, true, 3);
	c->Add(CancelBtn = new GButton(IDCANCEL, 392, 296, 70, 21, "Cancel"));

	// 5th row
	x = 0; y++;
	c = Tbl->GetCell(x++, y, true, 6);
	c->Add(ShowHidden = new GCheckBox(IDC_SHOWHIDDEN, 14, 326, -1, -1, "Show hidden files."));

	// Init
	if (BackBtn)
		BackBtn->Enabled(false);
	if (SaveBtn)
		SaveBtn->Enabled(false);
	if (FileLst)
		FileLst->MultiSelect(d->MultiSelect);
	if (ShowHidden)
		ShowHidden->Value(d->InitShowHiddenFiles);

	// Load types
	if (!d->Types.First())
	{
		GFileType *t = new GFileType;
		if (t)
		{
			t->Description("All Files");
			t->Extension(LGI_ALL_FILES);
			d->Types.Insert(t);
		}
	}
	for (GFileType *t=d->Types.First(); t; t=d->Types.Next())
	{
		char s[256];
		sprintf(s, "%s (%s)", t->Description(), t->Extension());
		if (FileTypeCbo)
			FileTypeCbo->Insert(s);
	}
	d->CurrentType = 0;

	// File + Path
	char *File = d->Files.First();
	if (File)
	{
		char *Dir = strrchr(File, DIR_CHAR);
		if (Dir)
		{
			OnFile(Dir + 1);
		}
		else
		{
			OnFile(File);
		}
	}
	
	if (d->InitPath)
	{
		SetFolder(d->InitPath);
	}
	else
	{
		char Str[256];
		LgiGetExePath(Str, sizeof(Str));
		SetFolder(Str);
	}
	OnFolder();

	// Size/layout
	SetPos(d->InitSize);
	MoveToCenter();

	RegisterHook(this, GKeyEvents);
	FileLst->Focus(true);
	
	LgiGetUsersLinks(Links);
	GTreeItem *RootItem = new GTreeItem;
	if (RootItem)
	{
		Bookmarks->Insert(RootItem);
		Add(RootItem, FileDev->GetRootVolume());
	}
	for (unsigned n=0; n<Links.Length(); n++)
	{
		GTreeItem *ci = new GTreeItem;
		if (ci)
		{
			char *p = Links[n];
			char *leaf = strrchr(p, DIR_CHAR);
			ci->SetText(leaf?leaf+1:p, 0);
			ci->SetText(p, 1);
			RootItem->Insert(ci);
		}
	}
}

GFileSelectDlg::~GFileSelectDlg()
{
	UnregisterHook(this);
	d->InitShowHiddenFiles = ShowHidden ? ShowHidden->Value() : false;
	d->InitSize = GetPos();

	char *CurPath = GetCtrlName(IDC_PATH);
	if (ValidStr(CurPath))
	{
		DeleteArray(d->InitPath);
		d->InitPath = NewStr(CurPath);
	}
}

void GFileSelectDlg::OnFile(char *f)
{
	if (d->Type != TypeOpenFolder)
	{
		FileNameEdit->Name(f ? f : (char*)"");
		SaveBtn->Enabled(ValidStr(f));
	}
}

void GFileSelectDlg::SetFolder(char *f)
{
	char *CurPath = GetCtrlName(IDC_PATH);
	if (CurPath)
	{
		d->History.Insert(NewStr(CurPath));
		SetCtrlEnabled(IDC_BACK, true);
	}
	
	SetCtrlName(IDC_PATH, f);
}

void GFileSelectDlg::OnFolder()
{
	if (Ctrl3)
		Ctrl3->OnFolder();
	if (FileLst)
		FileLst->OnFolder();

	char *CurPath = GetCtrlName(IDC_PATH);
	if (CurPath && UpBtn)
	{
		UpBtn->Enabled(strlen(CurPath)>3);
	}
}

void GFileSelectDlg::OnUpFolder()
{
	char *Cur = GetCtrlName(IDC_PATH);
	if (Cur)
	{
		char Dir[MAX_PATH];
		strcpy(Dir, Cur);
		if (strlen(Dir) > 3)
		{
			LgiTrimDir(Dir);

			if (!strchr(Dir, DIR_CHAR))
				strcat(Dir, DIR_STR);

			SetFolder(Dir);
			OnFolder();
		}
	}
}



void GFileSelectDlg::OnFilter(const char *Key)
{
	if (FileLst)
		FileLst->SetFilterKey(Key);
}

int GFileSelectDlg::OnNotify(GViewI *Ctrl, int Flags)
{
	switch (Ctrl->GetId())
	{
		case IDC_BOOKMARKS:
		{
			if (Flags == GNotifyItem_Select && Bookmarks)
			{
				GTreeItem *s = Bookmarks->Selection();
				if (s)
				{
					char *p = s->GetText(1);
					if (DirExists(p))
					{
						SetCtrlName(IDC_PATH, p);
						OnFolder();
					}
				}
			}
			break;
		}
		case IDC_PATH:
		{
			// if (Flags == VK_RETURN)
			if (Flags == GNotifyValueChanged)
			{
				// Skip the IDOK message generated by the default button
				// d->EatClose = true;
				// printf("%s:%i - eat close true\n", _FL);

				OnFolder();
			}
			break;
		}
		case IDC_VIEW:
		{
			if (FileLst)
			{
				/*	These functions are handled by the list 
					control's OnKey implementation
					
				if (Flags == GLIST_NOTIFY_RETURN)
				{
					List<LListItem> s;
					if (FileLst->GetSelection(s))
					{
						GFolderItem *i = dynamic_cast<GFolderItem*>(s.First());
						if (i)
						{
							i->OnActivate();
						}
					}
				}
				else if (Flags == GLIST_NOTIFY_BACKSPACE)
				{
					OnUpFolder();
				}
				*/
			}
			break;
		}
		case IDC_FILE:
		{
			char *f = Ctrl->Name();
			if (!f)
				break;

			if (Flags == VK_RETURN)
			{
				// allow user to insert new type by typing the pattern into the file name edit box and
				// hitting enter
				if (strchr(f, '?') || strchr(f, '*'))
				{
					// it's a mask, push the new type on the the type stack and
					// refilter the content
					int TypeIndex = -1;
					
					int n = 0;
					for (GFileType *t = d->Types.First(); t; t = d->Types.Next(), n++)
					{
						if (t->Extension() &&
							stricmp(t->Extension(), f) == 0)
						{
							TypeIndex = n;
							break;
						}
					}
					
					// insert the new type if not already there
					if (TypeIndex < 0)
					{
						GFileType *n = new GFileType;
						if (n)
						{
							n->Description(f);
							n->Extension(f);							
							TypeIndex = d->Types.Length();
							d->Types.Insert(n);
							FileTypeCbo->Insert(f);
						}
					}
					
					// select the new type
					if (TypeIndex >= 0)
					{
						FileTypeCbo->Value(d->CurrentType = TypeIndex);
					}
					
					// clear the edit box
					Ctrl->Name("");
					
					// Update and don't do normal save btn processing.
					OnFolder();
					
					// Skip the IDOK message generated by the default button
					d->EatClose = true;
					printf("%s:%i - eat close true\n", _FL);
					break;
				}
				
				if (DirExists(f))
				{
					// Switch to the folder...
					SetCtrlName(IDC_PATH, f);
					OnFolder();
					Ctrl->Name(NULL);
					d->EatClose = true;
				}
				else if (FileExists(f))
				{
					// Select the file...
					d->Files.Insert(NewStr(f));
					EndModal(IDOK);
					break;
				}
			}
			
			bool HasFile = ValidStr(f);
			bool BtnEnabled = SaveBtn->Enabled();
			if (HasFile ^ BtnEnabled)
			{
				SaveBtn->Enabled(HasFile);
			}
			break;
		}
		case IDC_BACK:
		{
			char *Dir = d->History.Last();
			if (Dir)
			{
				d->History.Delete(Dir);
				SetCtrlName(IDC_PATH, Dir);
				OnFolder();
				DeleteArray(Dir);

				if (!d->History.First())
				{
					SetCtrlEnabled(IDC_BACK, false);
				}
			}
			break;
		}
		case IDC_SHOWHIDDEN:
		{
			FileLst->OnFolder();
			break;
		}
		case IDC_TYPE:
		{
			d->CurrentType = FileTypeCbo->Value();
			FileLst->OnFolder();

			if (d->Type == TypeSaveFile)
			{
				// change extension of current file
				GFileType *Type = d->Types.ItemAt(d->CurrentType);
				char *File = FileNameEdit->Name();
				if (Type && File)
				{
					char *Ext = strchr(File, '.');
					if (Ext)
					{
						char *DefExt = Type->DefaultExtension();
						if (DefExt)
						{
							Ext++;
							char s[256];
							ZeroObj(s);
							memcpy(s, File, (int)Ext-(int)File);
							strcat(s, DefExt);

							OnFile(s);

							DeleteArray(DefExt);
						}
					}
				}
			}
			break;
		}
		case IDC_UP:
		{
			OnUpFolder();
			break;
		}
		case IDC_FILTER:
		{
			const char *n = Ctrl->Name();
			SetCtrlValue(IDC_FILTER_CLEAR, ValidStr(n));
			OnFilter(n);
			break;
		}
		case IDC_FILTER_CLEAR:
		{
			if (!Ctrl->Value())
			{
				SetCtrlName(IDC_FILTER, NULL);
				OnFilter(NULL);
			}
			break;
		}
		case IDC_NEW:
		{
			GInput Dlg(this, "", "Create new folder:", "New Folder");
			if (Dlg.DoModal())
			{
				char New[256];
				strcpy(New, GetCtrlName(IDC_PATH));
				if (New[strlen(New)-1] != DIR_CHAR) strcat(New, DIR_STR);
				strcat(New, Dlg.Str);

				FileDev->CreateFolder(New);
				OnFolder();
			}
			break;
		}
		case IDOK:
		{
			if (d->EatClose)
			{
				printf("%s:%i - SKIPPING eat close false\n", _FL);
				d->EatClose = false;
				break;
			}
			
			char *Path = GetCtrlName(IDC_PATH);
			char *File = GetCtrlName(IDC_FILE);
			if (Path)
			{
				char f[MAX_PATH];
				d->Files.DeleteArrays();

				if (d->Type == TypeOpenFolder)
				{
					d->Files.Insert(NewStr(Path));
				}
				else
				{
					List<LListItem> Sel;
					if (d->Type != TypeSaveFile &&
						FileLst &&
						FileLst->GetSelection(Sel) &&
						Sel.Length() > 1)
					{
						for (LListItem *i=Sel.First(); i; i=Sel.Next())
						{
							LgiMakePath(f, sizeof(f), Path, i->GetText(0));
							d->Files.Insert(NewStr(f));
						}
					}
					else if (ValidStr(File))
					{
						if (strchr(File, DIR_CHAR))
							strcpy_s(f, sizeof(f), File);
						else
							LgiMakePath(f, sizeof(f), Path, File);
						d->Files.Insert(NewStr(f));
					}
				}
			}

			// fall thru
		}
		case IDCANCEL:
		{
			EndModal(Ctrl->GetId());
			break;
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
class GFileSystemItem : public GTreeItem
{
	class GFileSystemPopup *Popup;
	char *Path;

public:
	GFileSystemItem(GFileSystemPopup *popup, GVolume *vol, char *path = 0);

	char *GetPath()
	{
		return Path;
	}

	void OnPath(char *p);
	void OnMouseClick(GMouse &m);
	bool OnKey(GKey &k);
};

#define IDC_TREE 100
class GFileSystemPopup : public GPopup
{
	friend class GFileSystemItem;

	GFileSelectDlg *Dlg;
	GTree *Tree;
	GFileSystemItem *Root;

public:
	GFileSystemPopup(GView *owner, GFileSelectDlg *dlg, int x) : GPopup(owner)
	{
		Dlg = dlg;
		GRect r(0, 0, x, 150);
		SetPos(r);
		Children.Insert(Tree = new GTree(IDC_TREE, 1, 1, X()-3, Y()-3));
		if (Tree)
		{
			Tree->Sunken(false);

			GVolume *v = FileDev->GetRootVolume();
			if (v)
			{
				Tree->SetImageList(Dlg->d->Icons, false);
				Tree->Insert(Root = new GFileSystemItem(this, v));
			}
		}
	}

	~GFileSystemPopup()
	{
	}

	void Visible(bool i)
	{
		if (i && Root)
		{
			Root->OnPath(Dlg->GetCtrlName(IDC_PATH));
		}

		GPopup::Visible(i);
	}

	void OnPaint(GSurface *pDC)
	{
		// Draw border
		GRect c = GetClient();
		c.Offset(-c.x1, -c.y1);
		pDC->Colour(LC_BLACK, 24);
		pDC->Box(&c);
		c.Size(1, 1);
	}

	void OnActivate(GFileSystemItem *i)
	{
		if (i)
		{
			Dlg->SetFolder(i->GetPath());
			Dlg->OnFolder();
			Visible(false);
		}
	}
};

GFileSystemItem::GFileSystemItem(GFileSystemPopup *popup, GVolume *Vol, char *path)
{
	Popup = popup;
	Expanded(true);

	if (Vol)
	{
		Path = NewStr(Vol->Path());
		SetText(Vol->Name());
		switch (Vol->Type())
		{
			case VT_3_5FLOPPY:
			case VT_5_25FLOPPY:
			case VT_REMOVABLE:
				SetImage(FSI_FLOPPY);
				break;
			case VT_HARDDISK:
			case VT_RAMDISK:
				SetImage(FSI_HARDDISK);
				break;
			case VT_CDROM:
				SetImage(FSI_CDROM);
				break;
			case VT_NETWORK_SHARE:
				SetImage(FSI_NETWORK);
				break;
			case VT_DESKTOP:
				SetImage(FSI_DESKTOP);
				break;
			default:
				SetImage(FSI_DIRECTORY);
				break;
		}

		for (GVolume *v=Vol->First(); v; v=Vol->Next())
		{
			Insert(new GFileSystemItem(Popup, v));
		}
	}
	else
	{
		Path = NewStr(path);
		SetText(strrchr(Path, DIR_CHAR)+1);
		SetImage(FSI_DIRECTORY);
	}
}

void GFileSystemItem::OnPath(char *p)
{
	switch (GetImage())
	{
		case FSI_DESKTOP:
		{
			if (p &&
				Path &&
				stricmp(Path, p) == 0)
			{
				Select(true);
				p = 0;
			}
			break;
		}
		case FSI_DIRECTORY:
		{
			return;
		}
		default:
		{
			GTreeItem *Old = Items.First();
			if (Old)
			{
				Old->Remove();
				DeleteObj(Old);
			}
			break;
		}
	}

	if (p)
	{
		int PathLen = strlen(Path);
		if (Path &&
			strnicmp(Path, p, PathLen) == 0 &&
			(p[PathLen] == DIR_CHAR || p[PathLen] == 0)
			#ifdef LINUX
			&& strcmp(Path, "/") != 0
			#endif
			)
		{
			GTreeItem *Item = this;

			if (GetImage() != FSI_DESKTOP &&
				strlen(p) > 3)
			{
				char *Start = p + strlen(Path);
				if (Start)
				{
					char s[256];
					strcpy(s, Path);

					GToken T(Start, DIR_STR);
					for (int i=0; i<T.Length(); i++)
					{
						if (s[strlen(s)-1] != DIR_CHAR) strcat(s, DIR_STR);
						strcat(s, T[i]);

						GFileSystemItem *New = new GFileSystemItem(Popup, 0, s);
						if (New)
						{
							Item->Insert(New);
							Item = New;
						}
					}
				}
			}

			if (Item)
			{
				Item->Select(true);
			}
		}
	}

	for (GFileSystemItem *i=dynamic_cast<GFileSystemItem*>(Items.First()); i;
						  i=dynamic_cast<GFileSystemItem*>(Items.Next()))
	{
		i->OnPath(p);
	}
}

void GFileSystemItem::OnMouseClick(GMouse &m)
{
	if (m.Left() && m.Down())
	{
		Popup->OnActivate(this);
	}
}

bool GFileSystemItem::OnKey(GKey &k)
{
	if ((k.c16 == ' ' || k.c16 == VK_RETURN))
	{
		if (k.Down() && k.IsChar)
		{
			Popup->OnActivate(this);
		}
		
		return true;
	}
	
	return false;
}

GFolderDrop::GFolderDrop(GFileSelectDlg *dlg, int Id, int x, int y, int cx, int cy) :
	GDropDown(Id, x, y, cx, cy, 0),
	GFolderView(dlg)
{
	SetPopup(new GFileSystemPopup(this, dlg, cx + (dlg->Ctrl2 ? dlg->Ctrl2->X() : 0) ));
}

void GFolderDrop::OnFolder()
{
}

//////////////////////////////////////////////////////////////////////////
#define IDM_OPEN				1000
#define IDM_CUT					1001
#define IDM_COPY				1002
#define IDM_RENAME				1003
#define IDM_PROPERTIES			1004
#define IDM_CREATE_SHORTCUT		1005
#define IDM_DELETE				1006

GFolderItem::GFolderItem(GFileSelectDlg *dlg, char *FullPath, GDirectory *Dir)
{
	Dlg = dlg;
	Path = NewStr(FullPath);
	File = strrchr(Path, DIR_CHAR);
	if (File) File++;
	IsDir = Dir->IsDir();
}

GFolderItem::~GFolderItem()
{
	DeleteArray(Path);
}

char *GFolderItem::GetText(int i)
{
	return File;
}

int GFolderItem::GetImage(int Flags)
{
	return IsDir ? 1 : 0;
}

void GFolderItem::OnSelect()
{
	if (!IsDir && File)
	{
		Dlg->OnFile(Select() ? File : 0);
	}
}

void GFolderItem::OnDelete(bool Ask)
{
	if (!Ask || LgiMsg(Parent, "Do you want to delete '%s'?", ModuleName, MB_YESNO, Path) == IDYES)
	{
		bool Status = false;

		if (IsDir)
		{
			Status = FileDev->RemoveFolder(Path, true);
		}
		else
		{
			Status = FileDev->Delete(Path);
		}

		if (Status)
		{
			Parent->Remove(this);
			delete this;
		}
	}
}

void GFolderItem::OnRename()
{
	GInput Inp(Dlg, File, "New name:", Dlg->Name());
	if (Inp.DoModal())
	{
		char Old[256];
		strcpy(Old, Path);

		char New[256];
		File[0] = 0;
		LgiMakePath(New, sizeof(New), Path, Inp.Str);
		
		if (FileDev->Move(Old, New))
		{
			DeleteArray(Path);
			Path = NewStr(New);
			File = strrchr(Path, DIR_CHAR);
			if (File) File++;
			Update();
		}
		else
		{
			LgiMsg(Dlg, "Renaming '%s' failed.", Dlg->Name(), MB_OK);
		}
	}
}

void GFolderItem::OnActivate()
{
	if (File)
	{
		if (IsDir)
		{
			char Dir[256];
			strcpy(Dir, Dlg->GetCtrlName(IDC_PATH));
			if (Dir[strlen(Dir)-1] != DIR_CHAR) strcat(Dir, DIR_STR);
			strcat(Dir, File);
			Dlg->SetFolder(Dir);
			Dlg->OnFolder();
		}
		else // Is file
		{
			Dlg->OnNotify(Dlg->SaveBtn, 0);
		}
	}
}

void GFolderItem::OnMouseClick(GMouse &m)
{
	if (m.Down())
	{
		if (m.Left())
		{
			if (m.Double())
			{
				OnActivate();
			}
		}
		else if (m.Right())
		{
			GSubMenu *RClick = new GSubMenu;
			if (RClick)
			{
				RClick->AppendItem("Select", IDM_OPEN, true);
				RClick->AppendSeparator();
				RClick->AppendItem("Cut", IDM_CUT, false);
				RClick->AppendItem("Copy", IDM_COPY, false);
				RClick->AppendSeparator();
				RClick->AppendItem("Create Shortcut", IDM_CREATE_SHORTCUT, false);
				RClick->AppendItem("Delete", IDM_DELETE, true);
				RClick->AppendItem("Rename", IDM_RENAME, true);
				RClick->AppendSeparator();
				RClick->AppendItem("Properties", IDM_PROPERTIES, false);

				if (Parent->GetMouse(m, true))
				{
					switch (RClick->Float(Parent, m.x, m.y))
					{
						case IDM_OPEN:
						{
							break;
						}
						case IDM_DELETE:
						{
							OnDelete();
							break;
						}
						case IDM_RENAME:
						{
							OnRename();
							break;
						}
					}
				}

				DeleteObj(RClick);
			}
		}
	}
}

int GFolderItemCompare(GFolderItem *a, GFolderItem *b, NativeInt Data)
{
	if (a && b)
	{
		if (a->IsDir ^ b->IsDir)
		{
			if (a->IsDir) return -1;
			else return 1;
		}
		else if (a->File && b->File)
		{
			return stricmp(a->File, b->File);
		}
	}

	return 0;
}

GFolderList::GFolderList(GFileSelectDlg *dlg, int Id, int x, int y, int cx, int cy) :
	LList(Id, x, y, cx, cy),
	GFolderView(dlg)
{
	SetImageList(Dlg->d->Icons, false);
	ShowColumnHeader(false);
	AddColumn("Name", cx-20);
	SetMode(LListColumns);
}

bool GFolderList::OnKey(GKey &k)
{
	bool Status = LList::OnKey(k);
	
	switch (k.vkey)
	{
		case VK_BACKSPACE:
		{
			if (k.Down() && GetWindow())
			{
				// Go up a directory
				GViewI *v = GetWindow()->FindControl(IDC_UP);
				if (v)
				{
					GetWindow()->OnNotify(v, 0);
				}
			}
			Status = true;
			break;
		}
		case VK_RETURN:
		#ifdef VK_KP_ENTER
		case VK_KP_ENTER:
		#endif
		{
			if (k.Down() && GetWindow())
			{
				GFolderItem *Sel = dynamic_cast<GFolderItem*>(GetSelected());
				if (Sel)
				{
					if (Sel->IsDir)
					{
						char *Cur = GetWindow()->GetCtrlName(IDC_PATH);
						if (Cur)
						{
							char Path[256];
							LgiMakePath(Path, sizeof(Path), Cur, Sel->GetText(0));
							if (DirExists(Path))
							{
								GetWindow()->SetCtrlName(IDC_PATH, Path);
								Dlg->OnFolder();
							}
						}
					}
					else
					{
						GViewI *Ok = GetWindow()->FindControl(IDOK);
						if (Ok)
						{
							GetWindow()->SetCtrlName(IDC_FILE, Sel->GetText(0));
							GetWindow()->OnNotify(Ok, 0);
						}
					}
				}
			}
			Status = true;
			break;
		}
		case VK_DELETE:
		{
			if (k.Down() && !k.IsChar && GetWindow())
			{
				List<LListItem> Sel;
				if (GetSelection(Sel))
				{
					GStringPipe Msg;
					Msg.Push("Do you want to delete:\n\n");
					
					List<GFolderItem> Delete;
					for (LListItem *i=Sel.First(); i; i=Sel.Next())
					{
						GFolderItem *s = dynamic_cast<GFolderItem*>(i);
						if (s)
						{
							Delete.Insert(s);
							Msg.Push("\t");
							Msg.Push(s->GetText(0));
							Msg.Push("\n");
						}
					}
					
					char *Mem = Msg.NewStr();
					if (Mem)
					{
						if (LgiMsg(this, Mem, ModuleName, MB_YESNO) == IDYES)
						{
							for (GFolderItem *d=Delete.First(); d; d=Delete.Next())
							{
								d->OnDelete(false);
							}
						}
						DeleteArray(Mem);
					}
				}
			}
			Status = true;
			break;
		}
	}

	// LgiTrace("%s:%i GFolderList::OnKey, key=%i down=%i status=%i\n", _FL, k.vkey, k.Down(), Status);
	
	return Status;
}

void GFolderList::OnFolder()
{
	Empty();

	GDirectory Dir;
	List<GFolderItem> New;

	// Get current type
	GFileType *Type = Dlg->d->Types.ItemAt(Dlg->d->CurrentType);
	List<char> Ext;
	if (Type)
	{
		GToken T(Type->Extension(), ";");
		for (int i=0; i<T.Length(); i++)
		{
			Ext.Insert(NewStr(T[i]));
		}
	}

	// Get items
	if (!Dlg->Ctrl2)
		return;
		
	bool ShowHiddenFiles = Dlg->ShowHidden ? Dlg->ShowHidden->Value() : false;
	for (bool Found = Dir.First(Dlg->Ctrl2->Name()); Found; Found = Dir.Next())
	{
		char Name[MAX_PATH];
		Dir.Path(Name, sizeof(Name));

		bool Match = true;
		if (!ShowHiddenFiles && Dir.IsHidden())
		{
			Match = false;
		}
		else if (!Dir.IsDir() &&
				Ext.Length() > 0)
		{
			Match = false;
			for (char *e=Ext.First(); e && !Match; e=Ext.Next())
			{
				if (e[0] == '*' && e[1] == '.')
				{
					char *Ext = LgiGetExtension(Name);
					if (Ext)
						Match = stricmp(Ext, e + 2) == 0;
				}
				else
				{
					bool m = MatchStr(e, Name);
					if (m)
						Match = true;
				}
			}
		}
		
		if (FilterKey && Match)
			Match = stristr(Dir.GetName(), FilterKey) != NULL;

		if (Match)
			New.Insert(new GFolderItem(Dlg, Name, &Dir));
	}

	// Sort items...
	New.Sort(GFolderItemCompare, 0);

	// Display items...
	Insert((List<LListItem>&) New);
}

//////////////////////////////////////////////////////////////////////////
GFileSelect::GFileSelect()
{
	d = new GFileSelectPrivate(this);
}

GFileSelect::~GFileSelect()
{
	DeleteObj(d);
}

void GFileSelect::ShowReadOnly(bool b)
{
	d->ShowReadOnly = b;;
}

bool GFileSelect::ReadOnly()
{
	return d->ReadOnly;
}

char *GFileSelect::Name()
{
	return d->Files.First();
}

bool GFileSelect::Name(const char *n)
{
	d->Files.DeleteArrays();
	if (n)
	{
		d->Files.Insert(NewStr(n));
	}

	return true;
}

char *GFileSelect::operator [](int i)
{
	return d->Files.ItemAt(i);
}

int GFileSelect::Length()
{
	return d->Files.Length();
}

int GFileSelect::Types()
{
	return d->Types.Length();
}

void GFileSelect::ClearTypes()
{
	d->Types.DeleteObjects();
}

GFileType *GFileSelect::TypeAt(int n)
{
	return d->Types.ItemAt(n);
}

bool GFileSelect::Type(const char *Description, const char *Extension, int Data)
{
	GFileType *Type = new GFileType;
	if (Type)
	{
		Type->Description(Description);
		Type->Extension(Extension);
		d->Types.Insert(Type);
	}

	return Type != 0;
}

int GFileSelect::SelectedType()
{
	return d->CurrentType;
}

GViewI *GFileSelect::Parent()
{
	return d->Parent;
}

void GFileSelect::Parent(GViewI *Window)
{
	d->Parent = dynamic_cast<GView*>(Window);
}

bool GFileSelect::MultiSelect()
{
	return d->MultiSelect;
}

void GFileSelect::MultiSelect(bool Multi)
{
	d->MultiSelect = Multi;
}

#define CharPropImpl(Func, Var)				\
	char *GFileSelect::Func()				\
	{										\
		return Var;							\
	}										\
	void GFileSelect::Func(const char *i)	\
	{										\
		DeleteArray(Var);					\
		if (i)								\
		{									\
			Var = NewStr(i);				\
		}									\
	}

CharPropImpl(InitialDir, d->InitPath);
CharPropImpl(Title, d->Title);
CharPropImpl(DefaultExtension, d->DefExt);

bool GFileSelect::Open()
{
	GFileSelectDlg Dlg(d);

	d->Type = TypeOpenFile;
	Dlg.Name("Open");
	if (Dlg.SaveBtn)
		Dlg.SaveBtn->Name("Open");

	return Dlg.DoModal() == IDOK;
}

bool GFileSelect::OpenFolder()
{
	GFileSelectDlg Dlg(d);

	d->Type = TypeOpenFolder;
	Dlg.SaveBtn->Enabled(true);
	Dlg.FileNameEdit->Enabled(false);
	Dlg.Name("Open Folder");
	Dlg.SaveBtn->Name("Open");

	return Dlg.DoModal() == IDOK;
}

bool GFileSelect::Save()
{
	GFileSelectDlg Dlg(d);
	
	d->Type = TypeSaveFile;
	Dlg.Name("Save As");
	Dlg.SaveBtn->Name("Save As");

	return Dlg.DoModal() == IDOK;
}

///////////////////////////////////////////////////////////////////////////////////
#if defined(LINUX)
#include "INet.h"
#endif
bool LgiGetUsersLinks(GArray<GString> &Links)
{
	GString Folder = LgiGetSystemPath(LSP_USER_LINKS);
	if (!Folder)
		return false;
	
	#if defined(WINDOWS)
		GDirectory d;
		for (int b = d.First(Folder); b; b = d.Next())
		{
			char *s = d.GetName();
			if (s && stristr(s, ".lnk"))
			{
				char lnk[MAX_PATH];
				if (d.Path(lnk, sizeof(lnk)) &&
					ResolveShortcut(lnk, lnk, sizeof(lnk)))
				{
					Links.New() = lnk;
				}
			}
		}		
	#elif defined(LINUX)
		char p[MAX_PATH];
		if (!LgiMakePath(p, sizeof(p), Folder, "bookmarks"))
		{
			LgiTrace("%s:%i - Failed to make path '%s'\n", _FL, Folder.Get());
			return false;
		}

		GAutoString Txt(ReadTextFile(p));
		if (!Txt)
		{
			LgiTrace("%s:%i - failed to read '%s'\n", _FL, p);
			return false;
		}

		GString s = Txt.Get();
		GString::Array a = s.Split("\n");
		for (unsigned i=0; i<a.Length(); i++)
		{
			GUri u(a[i]);
			if (u.Protocol && !_stricmp(u.Protocol, "file"))
				Links.New() = u.Path;
		}
	#else
		LgiAssert(!"Not impl yet.");
		return false;
	#endif
	
	return true;
}
