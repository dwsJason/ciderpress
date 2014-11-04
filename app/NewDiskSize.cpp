/*
 * CiderPress
 * Copyright (C) 2007 by faddenSoft, LLC.  All Rights Reserved.
 * See the file LICENSE for distribution terms.
 */
/*
 * Functions and data to support the "new disk size" radio buttons.
 */
#include "stdafx.h"
#include "NewDiskSize.h"
#include "resource.h"

/*
 * Number of blocks in the disks we create.
 *
 * These must be in ascending order.
 */
/*static*/ const NewDiskSize::RadioCtrlMap NewDiskSize::kCtrlMap[] = {
    { IDC_CONVDISK_140K,        280 },
    { IDC_CONVDISK_800K,        1600 },
    { IDC_CONVDISK_1440K,       2880 },
    { IDC_CONVDISK_5MB,         10240 },
    { IDC_CONVDISK_16MB,        32768 },
    { IDC_CONVDISK_20MB,        40960 },
    { IDC_CONVDISK_32MB,        65535 },
    { IDC_CONVDISK_SPECIFY,     kSpecified },
};
static const kEditBoxID = IDC_CONVDISK_SPECIFY_EDIT;

/*
 * Return the #of entries in the table.
 */
/*static*/ unsigned int
NewDiskSize::GetNumSizeEntries(void)
{
    return NELEM(kCtrlMap);
}

/*
 * Return the "size" field from an array entry.
 */
/*static*/ long
NewDiskSize::GetDiskSizeByIndex(int idx)
{
    ASSERT(idx >= 0 && idx < NELEM(kCtrlMap));
    return kCtrlMap[idx].blocks;
}

/*static*/ void
NewDiskSize::EnableButtons(CDialog* pDialog, BOOL state /*=true*/)
{
    CWnd* pWnd;

    for (int i = 0; i < NELEM(kCtrlMap); i++) {
        pWnd = pDialog->GetDlgItem(kCtrlMap[i].ctrlID);
        if (pWnd != nil)
            pWnd->EnableWindow(state);
    }
}

/*
 * Run through the set of radio buttons, disabling any that don't have enough
 * space to hold the ProDOS volume with the specified parameters.
 *
 * The space required is equal to the blocks required for data plus the blocks
 * required for the free-space bitmap.  Since the free-space bitmap size is
 * smaller for smaller volumes, we have to adjust it for each.
 *
 * Pass in the total blocks and #of blocks used on a particular ProDOS volume.
 * This will compute how much space would be required for larger and smaller
 * volumes, and enable or disable radio buttons as appropriate.  (You can get
 * these values from DiskFS::GetFreeBlockCount()).
 */
/*static*/ void
NewDiskSize::EnableButtons_ProDOS(CDialog* pDialog, long totalBlocks,
    long blocksUsed)
{
    CButton* pButton;
    long usedWithoutBitmap = blocksUsed - GetNumBitmapBlocks_ProDOS(totalBlocks);
    bool first = true;

    WMSG3("EnableButtons_ProDOS total=%ld used=%ld usedw/o=%ld\n",
        totalBlocks, blocksUsed, usedWithoutBitmap);

    for (int i = 0; i < NELEM(kCtrlMap); i++) {
        pButton = (CButton*) pDialog->GetDlgItem(kCtrlMap[i].ctrlID);
        if (pButton == nil) {
            WMSG1("WARNING: couldn't find ctrlID %d\n", kCtrlMap[i].ctrlID);
            continue;
        }

        if (kCtrlMap[i].blocks == kSpecified) {
            pButton->SetCheck(BST_UNCHECKED);
            pButton->EnableWindow(TRUE);
            CWnd* pWnd = pDialog->GetDlgItem(kEditBoxID);
            pWnd->EnableWindow(FALSE);
            continue;
        }

        if (usedWithoutBitmap + GetNumBitmapBlocks_ProDOS(kCtrlMap[i].blocks) <=
            kCtrlMap[i].blocks)
        {
            pButton->EnableWindow(TRUE);
            if (first) {
                pButton->SetCheck(BST_CHECKED);
                first = false;
            } else {
                pButton->SetCheck(BST_UNCHECKED);
            }
        } else {
            pButton->EnableWindow(FALSE);
            pButton->SetCheck(BST_UNCHECKED);
        }
    }

    UpdateSpecifyEdit(pDialog);
}

/*
 * Compute the #of blocks needed to hold the ProDOS block bitmap.
 */
/*static*/long
NewDiskSize::GetNumBitmapBlocks_ProDOS(long totalBlocks) {
    ASSERT(totalBlocks > 0);
    const int kBitsPerBlock = 512 * 8;
    int numBlocks = (totalBlocks + kBitsPerBlock-1) / kBitsPerBlock;
    return numBlocks;
}


/*
 * Update the "specify size" edit box.
 */
/*static*/ void
NewDiskSize::UpdateSpecifyEdit(CDialog* pDialog)
{
    CEdit* pEdit = (CEdit*) pDialog->GetDlgItem(kEditBoxID);
    int i;

    if (pEdit == nil) {
        ASSERT(false);
        return;
    }

    for (i = 0; i < NELEM(kCtrlMap); i++) {
        CButton* pButton = (CButton*) pDialog->GetDlgItem(kCtrlMap[i].ctrlID);
        if (pButton == nil) {
            WMSG1("WARNING: couldn't find ctrlID %d\n", kCtrlMap[i].ctrlID);
            continue;
        }

        if (pButton->GetCheck() == BST_CHECKED) {
            if (kCtrlMap[i].blocks == kSpecified)
                return;
            break;
        }
    }
    if (i == NELEM(kCtrlMap)) {
        WMSG0("WARNING: couldn't find a checked radio button\n");
        return;
    }

    CString fmt;
    fmt.Format("%ld", kCtrlMap[i].blocks);
    pEdit->SetWindowText(fmt);
}
