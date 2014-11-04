/*
 * CiderPress
 * Copyright (C) 2007 by faddenSoft, LLC.  All Rights Reserved.
 * See the file LICENSE for distribution terms.
 */
/*
 * Import BASIC programs stored in a text file.
 *
 * The current implementation is a bit lame.  It just dumps text strings into
 * a read-only edit buffer, instead of providing a nicer UI.  The real trouble
 * with this style of interface is that i18n is even more awkward.
 */
#include "StdAfx.h"
#include "../reformat/BASIC.h"
#include "BasicImport.h"
#include "HelpTopics.h"

/*
 * ==========================================================================
 *      BASTokenLookup
 * ==========================================================================
 */

/*
 * Constructor.  Pass in the info for the token blob.
 */
void
BASTokenLookup::Init(const char* tokenList, int numTokens,
    int tokenLen)
{
    int i;

    ASSERT(tokenList != nil);
    ASSERT(numTokens > 0);
    ASSERT(tokenLen > 0);

    delete[] fTokenPtr;     // in case we're being re-initialized
    delete[] fTokenLen;

    fTokenPtr = new const char*[numTokens];
    fTokenLen = new int[numTokens];
    fNumTokens = numTokens;

    for (i = 0; i < numTokens; i++) {
        fTokenPtr[i] = tokenList;
        fTokenLen[i] = strlen(tokenList);

        tokenList += tokenLen;
    }
}

/*
 * Return the index of the longest token that matches "str".
 *
 * Returns -1 if no match is found.
 */
int
BASTokenLookup::Lookup(const char* str, int len, int* pFoundLen)
{
    int longestIndex, longestLen;
    int i;

    longestIndex = longestLen = -1;
    for (i = 0; i < fNumTokens; i++) {
        if (fTokenLen[i] <= len && fTokenLen[i] > longestLen &&
            strncasecmp(str, fTokenPtr[i], fTokenLen[i]) == 0)
        {
            longestIndex = i;
            longestLen = fTokenLen[i];
        }
    }

    *pFoundLen = longestLen;
    return longestIndex;
}


/*
 * ==========================================================================
 *      ImportBASDialog
 * ==========================================================================
 */

BEGIN_MESSAGE_MAP(ImportBASDialog, CDialog)
    ON_COMMAND(IDHELP, OnHelp)
END_MESSAGE_MAP()


/*
 * Set up the dialog.
 */
BOOL
ImportBASDialog::OnInitDialog(void)
{
    CDialog::OnInitDialog();        // base class init

    PathName path(fFileName);
    CString fileNameOnly(path.GetFileName());
    CString ext(fileNameOnly.Right(4));
    if (ext.CompareNoCase(".txt") == 0) {
        WMSG1("removing extension from '%s'\n", (const char*) fileNameOnly);
        fileNameOnly = fileNameOnly.Left(fileNameOnly.GetLength() - 4);
    }

    CEdit* pEdit = (CEdit*) GetDlgItem(IDC_IMPORT_BAS_SAVEAS);
    pEdit->SetWindowText(fileNameOnly);
    pEdit->SetSel(0, -1);
    pEdit->SetFocus();

    /*
     * Do the actual import.  If it fails, disable the "save" button.
     */
    if (!ImportBAS(fFileName)) {
        CButton* pButton = (CButton*) GetDlgItem(IDOK);
        pButton->EnableWindow(FALSE);
        pEdit->EnableWindow(FALSE);
    }

    return FALSE;       // keep our focus
}

static const char* kFailed = "failed.\r\n\r\n";
static const char* kSuccess = "success!\r\n\r\n";

/*
 * Import an Applesoft BASIC program from the specified file.
 */
bool
ImportBASDialog::ImportBAS(const char* fileName)
{
    FILE* fp = NULL;
    ExpandBuffer msgs(1024);
    long fileLen, outLen, count;
    char* buf = nil;
    char* outBuf = nil;
    bool result = false;

    msgs.Printf("Importing from '%s'...", fileName);
    fp = fopen(fileName, "rb");     // EOL unknown, open as binary and deal
    if (fp == NULL) {
        msgs.Printf("%sUnable to open file.", kFailed);
        goto bail;
    }

    /* determine file length, and verify that it looks okay */
    fseek(fp, 0, SEEK_END);
    fileLen = ftell(fp);
    rewind(fp);
    if (ferror(fp) || fileLen < 0) {
        msgs.Printf("%sUnable to determine file length.", kFailed);
        goto bail;
    }
    if (fileLen == 0) {
        msgs.Printf("%sFile is empty.", kFailed);
        goto bail;
    }
    if (fileLen >= 128*1024) {
        msgs.Printf("%sFile is too large to be Applesoft.", kFailed);
        goto bail;
    }

    buf = new char[fileLen];
    if (buf == NULL) {
        msgs.Printf("%sUnable to allocate memory.", kFailed);
        goto bail;
    }

    /* read the entire thing into memory */
    count = fread(buf, 1, fileLen, fp);
    if (count != fileLen) {
        msgs.Printf("%sCould only read %ld of %ld bytes.", kFailed,
            count, fileLen);
        goto bail;
    }

    /* process it */
    if (!ConvertTextToBAS(buf, fileLen, &outBuf, &outLen, &msgs))
        goto bail;

    result = true;
    SetOutput(outBuf, outLen);

bail:
    if (fp != NULL)
        fclose(fp);
    delete[] buf;

    /* copy our error messages out */
    CEdit* pEdit = (CEdit*) GetDlgItem(IDC_IMPORT_BAS_RESULTS);
    char* msgBuf = nil;
    long msgLen;
    msgs.SeizeBuffer(&msgBuf, &msgLen);
    pEdit->SetWindowText(msgBuf);
    delete[] msgBuf;

    return result;
}

/*
 * Do the actual conversion.
 */
bool
ImportBASDialog::ConvertTextToBAS(const char* buf, long fileLen,
    char** pOutBuf, long* pOutLen, ExpandBuffer* pMsgs)
{
    ExpandBuffer output(32768);
    CString msg;
    const char* lineStart;
    const char* lineEnd;
    long textRemaining;
    int lineNum;

    fBASLookup.Init(ReformatApplesoft::GetApplesoftTokens(),
        ReformatApplesoft::kTokenCount, ReformatApplesoft::kTokenLen);

    lineEnd = buf;
    textRemaining = fileLen;
    lineNum = 0;

    while (textRemaining > 0) {
        lineNum++;
        lineStart = lineEnd;
        lineEnd = FindEOL(lineStart, textRemaining);

        if (!ProcessBASLine(lineStart, lineEnd - lineStart, &output,
            /*ref*/ msg))
        {
            pMsgs->Printf("%sLine %d: %s", kFailed, lineNum, (const char*) msg);
            return false;
        }

        textRemaining -= lineEnd - lineStart;
    }

    /* output EOF marker */
    output.Putc(0x00);
    output.Putc(0x00);

    /* grab the buffer */
    char* outBuf;
    long outLen;
    output.SeizeBuffer(&outBuf, &outLen);

    if (outLen >= 0xc000) {
        pMsgs->Printf("%sOutput is too large to be valid", kFailed);
        delete[] outBuf;
        return false;
    }

    /* go back and fix up the "next line" pointers, assuming a $0801 start */
    if (!FixBASLinePointers(outBuf, outLen, 0x0801)) {
        pMsgs->Printf("%sFailed while fixing line pointers", kFailed);
        delete[] outBuf;
        return false;
    }

    *pOutBuf = outBuf;
    *pOutLen = outLen;
    pMsgs->Printf("%sProcessed %d lines", kSuccess, lineNum);
    pMsgs->Printf("\r\nTokenized file is %d bytes long", *pOutLen);

    return true;
}

/*
 * Process a line of Applesoft BASIC text.
 *
 * Writes output to "pOutput".
 *
 * On failure, writes an error message to "msg" and returns false.
 */
/*
From an Applesoft disassembly by Bob Sander-Cederlof:

D56C- E8       1420 PARSE  INX          NEXT INPUT CHARACTER
D56D- BD 00 02 1430 .1     LDA INPUT.BUFFER,X
D570- 24 13    1440        BIT DATAFLG       IN A "DATA" STATEMENT?
D572- 70 04    1450        BVS .2            YES (DATAFLG = $49)
D574- C9 20    1460        CMP #' '     IGNORE BLANKS
D576- F0 F4    1470        BEQ PARSE
D578- 85 0E    1480 .2     STA ENDCHR
D57A- C9 22    1490        CMP #'"      START OF QUOTATION?
D57C- F0 74    1500        BEQ .13
D57E- 70 4D    1510        BVS .9       BRANCH IF IN "DATA" STATEMENT
D580- C9 3F    1520        CMP #'?      SHORTHAND FOR "PRINT"?
D582- D0 04    1530        BNE .3       NO
D584- A9 BA    1540        LDA #TOKEN.PRINT  YES, REPLACE WITH "PRINT" TOKEN
D586- D0 45    1550        BNE .9       ...ALWAYS
D588- C9 30    1560 .3     CMP #'0      IS IT A DIGIT, COLON, OR SEMI-COLON?
D58A- 90 04    1570        BCC .4       NO, PUNCTUATION !"#$%&'()*+,-./
D58C- C9 3C    1580        CMP #';'+1
D58E- 90 3D    1590        BCC .9       YES, NOT A TOKEN
               1600 *--------------------------------
               1610 *      SEARCH TOKEN NAME TABLE FOR MATCH STARTING
               1620 *      WITH CURRENT CHAR FROM INPUT LINE
               1630 *--------------------------------
D590- 84 AD    1640 .4     STY STRNG2   SAVE INDEX TO OUTPUT LINE
D592- A9 D0    1650        LDA #TOKEN.NAME.TABLE-$100
D594- 85 9D    1660        STA FAC      MAKE PNTR FOR SEARCH
D596- A9 CF    1670        LDA /TOKEN.NAME.TABLE-$100
D598- 85 9E    1680        STA FAC+1
D59A- A0 00    1690        LDY #0       USE Y-REG WITH (FAC) TO ADDRESS TABLE
D59C- 84 0F    1700        STY TKN.CNTR     HOLDS CURRENT TOKEN-$80
D59E- 88       1710        DEY          PREPARE FOR "INY" A FEW LINES DOWN
D59F- 86 B8    1720        STX TXTPTR   SAVE POSITION IN INPUT LINE
D5A1- CA       1730        DEX          PREPARE FOR "INX" A FEW LINES DOWN
D5A2- C8       1740 .5     INY          ADVANCE POINTER TO TOKEN TABLE
D5A3- D0 02    1750        BNE .6       Y=Y+1 IS ENOUGH
D5A5- E6 9E    1760        INC FAC+1    ALSO NEED TO BUMP THE PAGE
D5A7- E8       1770 .6     INX          ADVANCE POINTER TO INPUT LINE
D5A8- BD 00 02 1780 .7     LDA INPUT.BUFFER,X   NEXT CHAR FROM INPUT LINE
D5AB- C9 20    1790        CMP #' '     THIS CHAR A BLANK?
D5AD- F0 F8    1800        BEQ .6       YES, IGNORE ALL BLANKS
D5AF- 38       1810        SEC          NO, COMPARE TO CHAR IN TABLE
D5B0- F1 9D    1820        SBC (FAC),Y  SAME AS NEXT CHAR OF TOKEN NAME?
D5B2- F0 EE    1830        BEQ .5       YES, CONTINUE MATCHING
D5B4- C9 80    1840        CMP #$80     MAYBE; WAS IT SAME EXCEPT FOR BIT 7?
D5B6- D0 41    1850        BNE .14      NO, SKIP TO NEXT TOKEN
D5B8- 05 0F    1860        ORA TKN.CNTR     YES, END OF TOKEN; GET TOKEN #
D5BA- C9 C5    1870        CMP #TOKEN.AT  DID WE MATCH "AT"?
D5BC- D0 0D    1880        BNE .8       NO, SO NO AMBIGUITY
D5BE- BD 01 02 1890        LDA INPUT.BUFFER+1,X  "AT" COULD BE "ATN" OR "A TO"
D5C1- C9 4E    1900        CMP #'N      "ATN" HAS PRECEDENCE OVER "AT"
D5C3- F0 34    1910        BEQ .14      IT IS "ATN", FIND IT THE HARD WAY
D5C5- C9 4F    1920        CMP #'O      "TO" HAS PRECEDENCE OVER "AT"
D5C7- F0 30    1930        BEQ .14      IT IS "A TO", FIN IT THE HARD WAY
D5C9- A9 C5    1940        LDA #TOKEN.AT     NOT "ATN" OR "A TO", SO USE "AT"
               1950 *--------------------------------
               1960 *      STORE CHARACTER OR TOKEN IN OUTPUT LINE
               1970 *--------------------------------

Note the special handling for "AT" and "TO".  When it examines the next
character, it does NOT skip whitespace, making spaces significant when
differentiating between "at n"/"atn" and "at o"/"ato".
*/
bool
ImportBASDialog::ProcessBASLine(const char* buf, int len,
    ExpandBuffer* pOutput, CString& msg)
{
    const int kMaxTokenLen = 7;     // longest token; must also hold linenum
    const int kTokenAT = 0xc5 - 128;
    const int kTokenATN = 0xe1 - 128;
    char tokenBuf[kMaxTokenLen+1];
    bool gotOne = false;
    bool haveLineNum = false;
    char ch;
    int tokenLen;
    int lineNum;
    int foundToken;

    if (!len)
        return false;

    /*
     * Remove the CR, LF, or CRLF from the end of the line.
     */
    if (len > 1 && buf[len-2] == '\r' && buf[len-1] == '\n') {
        //WMSG0("removed CRLF\n");
        len -= 2;
    } else if (buf[len-1] == '\r') {
        //WMSG0("removed CR\n");
        len--;
    } else if (buf[len-1] == '\n') {
        //WMSG0("removed LF\n");
        len--;
    } else {
        //WMSG0("no EOL marker found\n");
    }

    if (!len)
        return true;        // blank lines are okay

    /*
     * Extract the line number.
     */
    tokenLen = 0;
    while (len > 0) {
        if (!GetNextNWC(&buf, &len, &ch)) {
            if (!gotOne)
                return true;        // blank lines with whitespace are okay
            else {
                // end of line reached while scanning line number is bad
                msg = "found nothing except line number";
                return false;
            }
        }
        gotOne = true;

        if (!isdigit(ch))
            break;
        if (tokenLen == 5) {    // theoretical max is "65535"
            msg = "line number has too many digits";
            return false;
        }
        tokenBuf[tokenLen++] = ch;
    }

    if (!tokenLen) {
        msg = "line did not start with a line number";
        return false;
    }
    tokenBuf[tokenLen] = '\0';
    lineNum = atoi(tokenBuf);
    WMSG1("FOUND line %d\n", lineNum);

    pOutput->Putc((char) 0xcc);     // placeholder
    pOutput->Putc((char) 0xcc);
    pOutput->Putc(lineNum & 0xff);
    pOutput->Putc((lineNum >> 8) & 0xff);

    /*
     * Start scanning tokens.
     *
     * We need to find the longest matching token (i.e. prefer "ONERR" over
     * "ON").  Grab a bunch of characters, ignoring whitespace, and scan
     * for a match.
     */
    buf--;          // back up
    len++;
    foundToken = -1;

    while (len > 0) {
        const char* dummy = buf;
        int remaining = len;

        /* load up the buffer */
        for (tokenLen = 0; tokenLen < kMaxTokenLen; tokenLen++) {
            if (!GetNextNWC(&dummy, &remaining, &ch))
                break;
            if (ch == '"')
                break;
            tokenBuf[tokenLen] = ch;
        }

        if (tokenLen == 0) {
            if (ch == '"') {
                /*
                 * Note it's possible for strings to be unterminated.  This
                 * will go unnoticed by Applesoft if it's at the end of a
                 * line.
                 */
                GetNextNWC(&buf, &len, &ch);
                pOutput->Putc(ch);
                while (len--) {
                    ch = *buf++;
                    pOutput->Putc(ch);
                    if (ch == '"')
                        break;
                }
            } else {
                /* end of line reached */
                break;
            }
        } else {
            int token, foundLen;

            token = fBASLookup.Lookup(tokenBuf, tokenLen, &foundLen);
            if (token >= 0) {
                /* match! */
                if (token == kTokenAT || token == kTokenATN) {
                    /* have to go back and re-scan original */
                    const char* tp = buf +1;
                    while (toupper(*tp++) != 'T')
                        ;
                    if (toupper(*tp) == 'N') {
                        /* keep this token */
                        assert(token == kTokenATN);
                    } else if (toupper(*tp) == 'O') {
                        /* eat and emit the 'A' so we get the "TO" instead */
                        goto output_single;
                    } else {
                        if (token == kTokenATN) {
                            /* reduce to "AT" */
                            token = kTokenAT;
                            foundLen--;
                        }
                    }
                }
                pOutput->Putc(token + 128);

                /* consume token chars, including whitespace */
                for (int j = 0; j < foundLen; j++)
                    GetNextNWC(&buf, &len, &ch);

                //WMSG2("TOKEN '%s' (%d)\n",
                //  fBASLookup.GetToken(token), tokenLen);

                /* special handling for REM or DATA */
                if (token == 0xb2 - 128) {
                    /* for a REM statement, copy verbatim to end of line */
                    if (*buf == ' ') {
                        /* eat one leading space, if present */
                        buf++;
                        len--;
                    }
                    while (len--) {
                        ch = *buf++;
                        pOutput->Putc(ch);
                    }
                } else if (token == 0x83 - 128) {
                    bool inQuote = false;

                    /* for a DATA statement, copy until ':' */
                    if (*buf == ' ') {
                        /* eat one leading space */
                        buf++;
                        len--;
                    }
                    while (len--) {
                        ch = *buf++;
                        if (ch == '"')  // ignore ':' in quoted strings
                            inQuote = !inQuote;

                        if (!inQuote && ch == ':') {
                            len++;
                            buf--;
                            break;
                        }
                        pOutput->Putc(ch);
                    }
                }
            } else {
                /*
                 * Not a quote, and no token begins with this character.
                 * Output it and advance.
                 */
output_single:
                GetNextNWC(&buf, &len, &ch);
                pOutput->Putc(toupper(ch));
            }
        }
    }

    pOutput->Putc('\0');

    return true;
}

/*
 * Fix up the line pointers.  We left dummy nonzero values in them initially.
 */
bool
ImportBASDialog::FixBASLinePointers(char* buf, long len, unsigned short addr)
{
    unsigned short val;
    char* start;

    while (len >= 4) {
        start = buf;
        val = (*buf) & 0xff | (*(buf+1)) << 8;

        if (val == 0)
            break;
        if (val != 0xcccc) {
            WMSG1("unexpected value 0x%04x found\n", val);
            return false;
        }

        buf += 4;
        len -= 4;

        /*
         * Find the next end-of-line marker.
         */
        while (*buf != '\0' && len > 0) {
            buf++;
            len--;
        }
        if (!len) {
            WMSG0("ran off the end?\n");
            return false;
        }
        buf++;
        len--;

        /*
         * Set the value.
         */
        val = (unsigned short) (buf - start);
        ASSERT((int) val == buf - start);
        addr += val;

        *start = addr & 0xff;
        *(start+1) = (addr >> 8) & 0xff;
    }

    return true;
}

/*
 * Look for the end of line.
 *
 * Returns a pointer to the first byte *past* the EOL marker, which will point
 * at unallocated space for last line in the buffer.
 */
const char*
ImportBASDialog::FindEOL(const char* buf, long max)
{
    ASSERT(max >= 0);
    if (max == 0)
        return nil;

    while (max) {
        if (*buf == '\r' || *buf == '\n') {
            if (*buf == '\r' && max > 0 && *(buf+1) == '\n')
                return buf+2;
            return buf+1;
        }

        buf++;
        max--;
    }

    /*
     * Looks like the last line didn't have an EOL.  That's okay.
     */
    return buf;
}

/*
 * Find the next non-whitespace character.
 *
 * Updates the buffer pointer and length.
 *
 * Returns "false" if we run off the end without finding another non-ws char.
 */
bool
ImportBASDialog::GetNextNWC(const char** pBuf, int* pLen, char* pCh)
{
    static const char* kWhitespace = " \t\r\n";

    while (*pLen > 0) {
        const char* ptr;
        char ch;

        ch = **pBuf;
        ptr = strchr(kWhitespace, ch);
        (*pBuf)++;
        (*pLen)--;

        if (ptr == nil) {
            *pCh = ch;
            return true;
        }
    }

    return false;
}


/*
 * Save the imported data.
 */
void ImportBASDialog::OnOK(void)
{
    CEdit* pEdit = (CEdit*) GetDlgItem(IDC_IMPORT_BAS_SAVEAS);
    CString fileName;

    pEdit->GetWindowText(fileName);
    if (fileName.IsEmpty()) {
        CString appName;
        appName.LoadString(IDS_MB_APP_NAME);

        MessageBox("You must specify a filename.",
            appName, MB_OK);
    }

    /*
     * Write the file to the currently-open archive.
     */
    GenericArchive::FileDetails details;

    details.entryKind = GenericArchive::FileDetails::kFileKindDataFork;
    details.origName = "Imported BASIC";
    details.storageName = fileName;
    details.access = 0xe3;  // unlocked, backup bit set
    details.fileType = kFileTypeBAS;
    details.extraType = 0x0801;
    details.storageType = DiskFS::kStorageSeedling;
    time_t now = time(nil);
    GenericArchive::UNIXTimeToDateTime(&now, &details.createWhen);
    GenericArchive::UNIXTimeToDateTime(&now, &details.archiveWhen);
    GenericArchive::UNIXTimeToDateTime(&now, &details.modWhen);

    CString errMsg;

    fDirty = true;
    if (!MainWindow::SaveToArchive(&details, (const unsigned char*) fOutput,
        fOutputLen, nil, -1, /*ref*/errMsg, this))
    {
        goto bail;
    }

    /* success! close the dialog */
    CDialog::OnOK();

bail:
    if (!errMsg.IsEmpty()) {
        CString msg;
        msg.Format("Unable to import file: %s.", (const char *) errMsg);
        ShowFailureMsg(this, msg, IDS_FAILED);
        return;
    }
    return;
}

/*
 * User pressed the "Help" button.
 */
void
ImportBASDialog::OnHelp(void)
{
    WinHelp(HELP_TOPIC_IMPORT_BASIC, HELP_CONTEXT);
}
