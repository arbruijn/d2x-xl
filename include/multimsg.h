#ifndef __MULTIMSG_H
#define __MULTIMSG_H

void MultiSendMessage (void);
void MultiDefineMacro (int32_t key);
int32_t MultiMessageFeedback (void);
void MultiSendMacro (int32_t key);
void MultiDoStartTyping (char *buf);
void MultiDoQuitTyping (char *buf);
void MultiSendTyping (void);
void MultiSendMsgStart (char nMsg);
void MultiSendMsgQuit ();
int32_t KickPlayer (int32_t bBan);
int32_t PingPlayer (int32_t i);
int32_t HandicapPlayer (void);
int32_t MovePlayer (void);
void MultiSendMsgEnd ();
void MultiDefineMacroEnd ();
void MultiMsgInputSub (int32_t key);
void MultiSendMsgDialog (void);
void MultiDoMsg (char *buf);

#endif //__MULTIMSG_H
//eof
