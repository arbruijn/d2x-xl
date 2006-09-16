#ifndef __MULTIMSG_H
#define __MULTIMSG_H

void MultiSendMessage (void);
void MultiDefineMacro (int key);
int MultiMessageFeedback (void);
void MultiSendMacro (int key);
void MultiDoStartTyping (char *buf);
void MultiDoQuitTyping (char *buf);
void MultiSendTyping (void);
void MultiSendMsgStart (char nMsg);
void MultiSendMsgQuit ();
int KickPlayer (int bBan);
int PingPlayer (int i);
int HandicapPlayer (void);
int MovePlayer (void);
void MultiSendMsgEnd ();
void MultiDefineMacroEnd ();
void MultiMsgInputSub (int key);
void MultiSendMsgDialog (void);
void MultiDoMsg (char *buf);

#endif //__MULTIMSG_H
//eof
