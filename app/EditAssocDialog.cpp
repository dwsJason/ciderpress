/*
 * CiderPress
 * Copyright (C) 2007 by faddenSoft, LLC.  All Rights Reserved.
 * See the file LICENSE for distribution terms.
 */
/*
 * Support for EditAssocDialog.
 */
#include "stdafx.h"
#include "EditAssocDialog.h"
#include "MyApp.h"
#include "Registry.h"
#include "HelpTopics.h"

BEGIN_MESSAGE_MAP(EditAssocDialog, CDialog)
    ON_WM_HELPINFO()
    ON_COMMAND(IDHELP, OnHelp)
END_MESSAGE_MAP()

/* this comes from VC++6.0 MSDN help */
#ifndef ListView_SetCheckState
   #define ListView_SetCheckState(hwndLV, i, fCheck) \
      ListView_SetItemState(hwndLV, i, \
      INDEXTOSTATEIMAGEMASK((fCheck)+1), LVIS_STATEIMAGEMASK)
#endif

/*
 * Tweak the controls.
 */
BOOL
EditAssocDialog::OnInitDialog(void)
{
    CListCtrl* pListView = (CListCtrl*) GetDlgItem(IDC_ASSOCIATION_LIST);

    ASSERT(pListView != nil);
    //pListView->ModifyStyleEx(0, LVS_EX_CHECKBOXES);
    ListView_SetExtendedListViewStyleEx(pListView->m_hWnd,
        LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);

    /* move it over slightly so we see some overlap */
    CRect rect;
    GetWindowRect(&rect);
    rect.left += 10;
    rect.right += 10;
    MoveWindow(&rect);


    /*
     * Initialize this before DDX stuff happens.  If the caller didn't
     * provide a set, load our own.
     */
    if (fOurAssociations == nil) {
        fOurAssociations = new bool[gMyApp.fRegistry.GetNumFileAssocs()];
        Setup(true);
    } else {
        Setup(false);
    }

    return CDialog::OnInitDialog();
}

/*
 * Load the list view control.
 *
 * This list isn't sorted, so we don't need to stuff anything into lParam to
 * keep the list and source data tied.
 *
 * If "loadAssoc" is true, we also populate the fOurAssocations table.
 */
void
EditAssocDialog::Setup(bool loadAssoc)
{
    WMSG0("Setup!\n");

    CListCtrl* pListView = (CListCtrl*) GetDlgItem(IDC_ASSOCIATION_LIST);
    ASSERT(pListView != nil);

    ASSERT(fOurAssociations != nil);

    /* two columns */
    CRect rect;
    pListView->GetClientRect(&rect);
    int width;

    width = pListView->GetStringWidth("XXExtensionXX");
    pListView->InsertColumn(0, "Extension", LVCFMT_LEFT, width);
    pListView->InsertColumn(1, "Association", LVCFMT_LEFT,
        rect.Width() - width);

    int num = gMyApp.fRegistry.GetNumFileAssocs();
    int idx = 0;
    while (num--) {
        CString ext, handler;
        CString dispStr;
        bool ours;

        gMyApp.fRegistry.GetFileAssoc(idx, &ext, &handler, &ours);

        pListView->InsertItem(idx, ext);
        pListView->SetItemText(idx, 1, handler);

        if (loadAssoc)
            fOurAssociations[idx] = ours;
        idx++;
    }

    //DeleteAllItems(); // for Reload case
}

/*
 * Copy state in and out of dialog.
 */
void
EditAssocDialog::DoDataExchange(CDataExchange* pDX)
{
    CListCtrl* pListView = (CListCtrl*) GetDlgItem(IDC_ASSOCIATION_LIST);

    ASSERT(fOurAssociations != nil);
    if (fOurAssociations == nil)
        return;

    int num = gMyApp.fRegistry.GetNumFileAssocs();

    if (!pDX->m_bSaveAndValidate) {
        /* load fixed set of file associations */
        int idx = 0;
        while (num--) {
            ListView_SetCheckState(pListView->m_hWnd, idx,
                fOurAssociations[idx]);
            idx++;
        }
    } else {
        /* copy the checkboxes out */
        int idx = 0;
        while (num--) {
            fOurAssociations[idx] =
                        (ListView_GetCheckState(pListView->m_hWnd, idx) != 0);
            idx++;
        }
    }
}

/*
 * Context help request (question mark button).
 */
BOOL
EditAssocDialog::OnHelpInfo(HELPINFO* lpHelpInfo)
{
    return ShowContextHelp(this, lpHelpInfo);
}

/*
 * User pressed the "Help" button.
 */
void
EditAssocDialog::OnHelp(void)
{
    WinHelp(HELP_TOPIC_EDIT_ASSOC, HELP_CONTEXT);
}
