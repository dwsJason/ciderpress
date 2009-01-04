/*
 * CiderPress
 * Copyright (C) 2007 by faddenSoft, LLC.  All Rights Reserved.
 * See the file LICENSE for distribution terms.
 */
/*
 * Support for "choose a directory" dialog.
 */
#include "stdafx.h"
#include "ChooseDirDialog.h"
#include "NewFolderDialog.h"
#include "DiskFSTree.h"
#include "HelpTopics.h"

BEGIN_MESSAGE_MAP(ChooseDirDialog, CDialog)
	ON_NOTIFY(TVN_SELCHANGED, IDC_CHOOSEDIR_TREE, OnSelChanged)
	ON_BN_CLICKED(IDC_CHOOSEDIR_EXPAND_TREE, OnExpandTree)
	ON_BN_CLICKED(IDC_CHOOSEDIR_NEW_FOLDER, OnNewFolder)
	ON_WM_HELPINFO()
	//ON_COMMAND(ID_HELP, OnIDHelp)
	ON_BN_CLICKED(IDHELP, OnHelp)
END_MESSAGE_MAP()


/*
 * Initialize dialog components.
 */
BOOL
ChooseDirDialog::OnInitDialog(void)
{
	CDialog::OnInitDialog();

	/* set up the "new folder" button */
	fNewFolderButton.ReplaceDlgCtrl(this, IDC_CHOOSEDIR_NEW_FOLDER);
	fNewFolderButton.SetBitmapID(IDB_NEW_FOLDER);

	/* replace the tree control with a ShellTree */
	if (fShellTree.ReplaceDlgCtrl(this, IDC_CHOOSEDIR_TREE) != TRUE) {
		WMSG0("WARNING: ShellTree replacement failed\n");
		ASSERT(false);
	}

	//enable images
	fShellTree.EnableImages();
	//populate for the with Shell Folders for the first time
	fShellTree.PopulateTree(/*CSIDL_DRIVES*/);

	if (fPathName.IsEmpty()) {
		// start somewhere reasonable
		fShellTree.ExpandMyComputer();
	} else {
		CString msg("");
		fShellTree.TunnelTree(fPathName, &msg);
		if (!msg.IsEmpty()) {
			/* failed */
			WMSG2("TunnelTree failed on '%s' (%s), using MyComputer instead\n",
				fPathName, msg);
			fShellTree.ExpandMyComputer();
		}
	}

	fShellTree.SetFocus();
	return FALSE;	// leave focus on shell tree
}

/*
 * Special keypress handling.
 */
BOOL
ChooseDirDialog::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN &&
         pMsg->wParam == VK_RETURN)
	{
		//WMSG0("RETURN!\n");
		if (GetFocus() == GetDlgItem(IDC_CHOOSEDIR_PATHEDIT)) {
			OnExpandTree();
			return TRUE;
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

/*
 * F1 key hit, or '?' button in title bar used to select help for an
 * item in the dialog.
 */
BOOL
ChooseDirDialog::OnHelpInfo(HELPINFO* lpHelpInfo)
{
	DWORD context = lpHelpInfo->iCtrlId;
	WinHelp(context, HELP_CONTEXTPOPUP);
	return TRUE;	// indicate success??
}

/*
 * User pressed Ye Olde Helppe Button.
 */
void
ChooseDirDialog::OnHelp(void)
{
	WinHelp(HELP_TOPIC_CHOOSE_FOLDER, HELP_CONTEXT);
}

/*
 * Replace the ShellTree's default SELCHANGED handler with this so we can
 * track changes to the edit control.
 */
void
ChooseDirDialog::OnSelChanged(NMHDR* pnmh, LRESULT* pResult)
{
	CString path;
	CWnd* pWnd = GetDlgItem(IDC_CHOOSEDIR_PATH);
	ASSERT(pWnd != nil);

	if (fShellTree.GetFolderPath(&path))
		fPathName = path;
	else
		fPathName = "";
	pWnd->SetWindowText(fPathName);

	// disable the "Select" button when there's no path ready
	pWnd = GetDlgItem(IDOK);
	ASSERT(pWnd != nil);
	pWnd->EnableWindow(!fPathName.IsEmpty());

	// It's confusing to have two different paths showing, so wipe out the
	// free entry field when the selection changes.
	pWnd = GetDlgItem(IDC_CHOOSEDIR_PATHEDIT);
	pWnd->SetWindowText("");

	*pResult = 0;
}

/*
 * User pressed "Expand Tree" button.
 */
void
ChooseDirDialog::OnExpandTree(void)
{
	CWnd* pWnd;
	CString str;
	CString msg;

	pWnd = GetDlgItem(IDC_CHOOSEDIR_PATHEDIT);
	ASSERT(pWnd != nil);
	pWnd->GetWindowText(str);

	if (!str.IsEmpty()) {
		fShellTree.TunnelTree(str, &msg);
		if (!msg.IsEmpty()) {
			CString failed;
			failed.LoadString(IDS_FAILED);
			MessageBox(msg, failed, MB_OK | MB_ICONERROR);
		}
	}
}

/*
 * User pressed "New Folder" button.
 */
void
ChooseDirDialog::OnNewFolder(void)
{
	if (fPathName.IsEmpty()) {
		MessageBox("You can't create a folder in this part of the tree.",
			"Bad Location", MB_OK | MB_ICONERROR);
		return;
	}

	NewFolderDialog newFolderDlg;

	newFolderDlg.fCurrentFolder = fPathName;
	if (newFolderDlg.DoModal() == IDOK) {
		if (newFolderDlg.GetFolderCreated()) {
			/*
			 * They created a new folder.  We want to add it to the tree
			 * and then select it.  This is not too hard because we know
			 * that the folder was created under the currently-selected
			 * tree node.
			 */
			if (fShellTree.AddFolderAtSelection(newFolderDlg.fNewFolder)) {
				CString msg;
				WMSG1("Success, tunneling to '%s'\n",
					newFolderDlg.fNewFullPath);
				fShellTree.TunnelTree(newFolderDlg.fNewFullPath, &msg);
				if (!msg.IsEmpty()) {
					WMSG1("TunnelTree failed: %s\n", (LPCTSTR) msg);
				}
			} else {
				WMSG0("AddFolderAtSelection FAILED\n");
				ASSERT(false);
			}
		} else {
			WMSG0("NewFolderDialog returned IDOK but no create\n");
		}
	}
}