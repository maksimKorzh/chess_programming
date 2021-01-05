#pragma once
/* *INDENT-OFF* */
/*>>> epd.h: subprogram prototypes for epd.c */
/* Revised: 1996.06.23 */
/*
   Copyright (C) 1996 by Steven J. Edwards (sje@mv.mv.com)
   All rights reserved.  This code may be freely redistibuted and used by
   both research and commerical applications.  No warranty exists.
 */
/*
   Everything in this source file is independent of the host program.
   Requests for changes and additions should be communicated to the author
   via the e-mail address given above.
 */
/*
   This file was originally prepared on an Apple Macintosh using the
   Metrowerks CodeWarrior 6 ANSI C compiler.  Tabs are set at every
   four columns.  Further testing and development was performed on a
   generic PC running Linux 1.3.20 and using the gcc 2.7.0 compiler.
 */
void EPDFatal(charptrT s);
void EPDSwitchFault(charptrT s);
voidptrT EPDMemoryGrab(liT n);
void EPDMemoryFree(voidptrT ptr);
charptrT EPDStringGrab(charptrT s);
void EPDStringFree(charptrT s);
charptrT EPDStringAppendChar(charptrT s, char c);
charptrT EPDStringAppendStr(charptrT s0, charptrT s1);
liT EPDMapFromDuration(charptrT s);
charptrT EPDMapToDuration(liT seconds);
gamptrT EPDGameOpen(void);
void EPDGameClose(gamptrT gamptr);
void EPDGameAppendMove(gamptrT gamptr, mptrT mptr);
void EPDTokenize(charptrT s);
siT EPDTokenCount(void);
charptrT EPDTokenFetch(siT n);
siT EPDCICharEqual(char ch0, char ch1);
pT EPDPieceFromCP(cpT cp);
siT EPDCheckPiece(char ch);
pT EPDEvaluatePiece(char ch);
siT EPDCheckColor(char ch);
cT EPDEvaluateColor(char ch);
siT EPDCheckRank(char ch);
rankT EPDEvaluateRank(char ch);
siT EPDCheckFile(char ch);
fileT EPDEvaluateFile(char ch);
eovptrT EPDNewEOV(void);
void EPDReleaseEOV(eovptrT eovptr);
void EPDAppendEOV(eopptrT eopptr, eovptrT eovptr);
eovptrT EPDCreateEOVStr(charptrT str);
eovptrT EPDCreateEOVSym(charptrT sym);
eovptrT EPDCreateEOVInt(liT lval);
eovptrT EPDLocateEOV(eopptrT eopptr, charptrT strval);
siT EPDCountEOV(eopptrT eopptr);
void EPDReplaceEOVStr(eovptrT eovptr, charptrT str);
eopptrT EPDNewEOP(void);
void EPDReleaseEOP(eopptrT eopptr);
void EPDAppendEOP(epdptrT epdptr, eopptrT eopptr);
eopptrT EPDCreateEOP(charptrT opsym);
eopptrT EPDCreateEOPCode(epdsoT epdso);
eopptrT EPDLocateEOP(epdptrT epdptr, charptrT opsym);
eopptrT EPDLocateEOPCode(epdptrT epdptr, epdsoT epdso);
siT EPDCountEOP(epdptrT epdptr);
void EPDDropIfLocEOP(epdptrT epdptr, charptrT opsym);
void EPDDropIfLocEOPCode(epdptrT epdptr, epdsoT epdso);
void EPDAddOpInt(epdptrT epdptr, epdsoT epdso, liT val);
void EPDAddOpStr(epdptrT epdptr, epdsoT epdso, charptrT s);
void EPDAddOpSym(epdptrT epdptr, epdsoT epdso, charptrT s);
epdptrT EPDNewEPD(void);
void EPDReleaseOperations(epdptrT epdptr);
void EPDReleaseEPD(epdptrT epdptr);
charptrT EPDFetchOpsym(epdsoT epdso);
epdptrT EPDCloneEPDBase(epdptrT epdptr);
eovptrT EPDCloneEOV(eovptrT eovptr);
eopptrT EPDCloneEOP(eopptrT eopptr);
epdptrT EPDCloneEPD(epdptrT epdptr);
epdptrT EPDSet(rbptrT rbptr, cT actc, castT cast, sqT epsq);
void EPDSetCurrentPosition(rbptrT rbptr, cT actc, castT cast, sqT epsq,
    siT hmvc, siT fmvn);
epdptrT EPDGetCurrentPosition(void);
cT EPDFetchACTC(void);
castT EPDFetchCAST(void);
sqT EPDFetchEPSQ(void);
siT EPDFetchHMVC(void);
siT EPDFetchFMVN(void);
rbptrT EPDFetchBoard(void);
cpT EPDFetchCP(sqT sq);
charptrT EPDFetchBoardString(void);
gtimT EPDGetGTIM(gamptrT gamptr);
void EPDPutGTIM(gamptrT gamptr, gtimT gtim);
charptrT EPDGenBasic(rbptrT rbptr, cT actc, castT cast, sqT epsq);
charptrT EPDGenBasicCurrent(void);
epdptrT EPDDecodeFEN(charptrT s);
charptrT EPDEncodeFEN(epdptrT epdptr);
epdptrT EPDDecode(charptrT s);
charptrT EPDEncode(epdptrT epdptr);
void EPDRealize(epdptrT epdptr);
void EPDInitArray(void);
charptrT EPDPlayerString(cT c);
void EPDSANEncode(mptrT mptr, sanT san);
mptrT EPDSANDecodeAux(sanT san, siT strict);
siT EPDIsLegal(void);
siT EPDIsCheckmate(void);
siT EPDIsStalemate(void);
siT EPDIsInsufficientMaterial(void);
siT EPDIsFiftyMoveDraw(void);
siT EPDIsThirdRepeatIndex(gamptrT gamptr);
siT EPDIsDraw(gamptrT gamptr);
mptrT EPDMateInOne(void);
void EPDExecuteUpdate(mptrT mptr);
void EPDRetractUpdate(void);
void EPDRetractAll(void);
void EPDCollapse(void);
void EPDReset(void);
void EPDGenMoves(void);
siT EPDFetchMoveCount(void);
mptrT EPDFetchMove(siT index);
void EPDSetMoveFlags(mptrT mptr);
void EPDSortSAN(void);
siT EPDPurgeOpFile(charptrT opsym, charptrT fn0, charptrT fn1);
siT EPDRepairEPD(epdptrT epdptr);
siT EPDRepairFile(charptrT fn0, charptrT fn1);
siT EPDNormalizeFile(charptrT fn0, charptrT fn1);
siT EPDScoreFile(charptrT fn, bmsptrT bmsptr);
siT EPDEnumerateFile(siT depth, charptrT fn0, charptrT fn1, liptrT totalptr);
charptrT EPDMoveList(gamptrT gamptr);
pgnstrT EPDPGNFetchTagIndex(charptrT s);
charptrT EPDPGNFetchTagName(pgnstrT pgnstr);
charptrT EPDPGNGetSTR(gamptrT gamptr, pgnstrT pgnstr);
void EPDPGNPutSTR(gamptrT gamptr, pgnstrT pgnstr, charptrT s);
charptrT EPDPGNGenSTR(gamptrT gamptr);
charptrT EPDPGNHistory(gamptrT gamptr);
void EPDCopyInPTP(gamptrT gamptr, epdptrT epdptr);
void EPDCopyOutPTP(gamptrT gamptr, epdptrT epdptr);
charptrT EPDFetchRefcomStr(refcomT refcom);
charptrT EPDFetchRefreqStr(refreqT refreq);
refcomT EPDFetchRefcomIndex(charptrT s);
refreqT EPDFetchRefreqIndex(charptrT s);
refcomT EPDExtractRefcomIndex(epdptrT epdptr);
refreqT EPDExtractRefreqIndex(epdptrT epdptr);
siT EPDComm(refintptrT refintptr, charptrT pipebase);
void EPDInit(void);
void EPDTerm(void);
/* *INDENT-ON* */
