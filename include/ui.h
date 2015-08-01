/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _UI_H
#define _UI_H

typedef struct {
	char	description[100];
	char 	* buttontext[17];
	int32_t	numkeys;
	int16_t keyCode [100];
	int32_t 	function_number[100];
} UI_KEYPAD;

typedef struct
{
	uint32_t frame;
	int32_t nType;
	int32_t data;
} UI_EVENT;

#define BASE_GADGET             \
	int16_t           kind;       \
	struct gadget  * prev;     \
	struct gadget  * next;     \
	struct gadget  * when_tab;  \
	struct gadget  * when_btab; \
	struct gadget  * when_up;    \
	struct gadget  * whenDown;   \
	struct gadget  * when_left;   \
	struct gadget  * when_right;  \
	struct gadget  * parent;    \
	int32_t             status;     \
	int32_t             oldstatus;  \
	CCanvas *    canvas;     \
	int32_t             hotkey;     \
	int16_t           x1,y1,x2,y2;


typedef struct gadget {
	BASE_GADGET
	uint8_t rsvd[256];
} UI_GADGET;


typedef struct  {
	BASE_GADGET
	int32_t         trap_key;
	int32_t      (*user_function)(void);
} UI_GADGET_KEYTRAP;

typedef struct  {
	BASE_GADGET
	int16_t           width, height;
	int16_t           b1_heldDown;
	int16_t           b1_clicked;
	int16_t           b1_double_clicked;
	int16_t           b1_dragging;
	int16_t           b1_drag_x1, b1_drag_y1;
	int16_t           b1_drag_x2, b1_drag_y2;
	int16_t           b1_done_dragging;
	int32_t             keypress;
	int16_t           mouse_onme;
	int16_t           mouse_x, mouse_y;
	CBitmap *    bitmap;
} UI_GADGET_USERBOX;

typedef struct  {
	BASE_GADGET
	int16_t           width, height;
	char            * text;
	int16_t           position;
	int16_t           oldposition;
	int16_t           pressed;
	int32_t          	 (*user_function)(void);
	int32_t          	 (*user_function1)(void);
	int32_t				 hotkey1;
	int32_t				 dim_if_no_function;
} UI_GADGET_BUTTON;


typedef struct  {
	BASE_GADGET
	int16_t           width, height;
	char            * text;
	int16_t           length;
	int16_t           slength;
	int16_t           position;
	int16_t           oldposition;
	int16_t           pressed;
	int16_t           firstTime;
} UI_GADGET_INPUTBOX;

typedef struct  {
	BASE_GADGET
	int16_t           width, height;
	char            * text;
	int16_t           position;
	int16_t           oldposition;
	int16_t           pressed;
	int16_t           group;
	int16_t           flag;
} UI_GADGET_RADIO;


typedef struct  {
	BASE_GADGET
	char 				 *text;
	int16_t 		    width, height;
	uint8_t           flag;
	uint8_t           pressed;
	uint8_t           position;
	uint8_t           oldposition;
	int32_t             trap_key;
	int32_t          	(*user_function)(void);
} UI_GADGET_ICON;


typedef struct  {
	BASE_GADGET
	int16_t           width, height;
   char            * text;
	int16_t           position;
	int16_t           oldposition;
	int16_t           pressed;
	int16_t           group;
	int16_t           flag;
} UI_GADGET_CHECKBOX;


typedef struct  {
	BASE_GADGET
	int16_t           horz;
	int16_t           width, height;
	int32_t             start;
	int32_t             stop;
	int32_t             position;
	int32_t             window_size;
	int32_t             fake_length;
	int32_t             fake_position;
	int32_t             fake_size;
	UI_GADGET_BUTTON * up_button;
	UI_GADGET_BUTTON * down_button;
	uint32_t    last_scrolled;
	int16_t           drag_x, drag_y;
	int32_t             drag_starting;
	int32_t             dragging;
	int32_t             moved;
} UI_GADGET_SCROLLBAR;

typedef struct  {
	BASE_GADGET
	int16_t           width, height;
	char            * list;
	int32_t             text_width;
	int32_t             num_items;
	int32_t             num_items_displayed;
	int32_t             first_item;
	int32_t             old_first_item;
	int32_t             current_item;
	int32_t             selected_item;
	int32_t             old_current_item;
	uint32_t    last_scrolled;
	int32_t             dragging;
	int32_t             textheight;
	UI_GADGET_SCROLLBAR * scrollbar;
	int32_t             moved;
} UI_GADGET_LISTBOX;

typedef struct ui_window {
	int16_t           x, y;
	int16_t           width, height;
	int16_t           text_x, text_y;
	CCanvas *    canvas;
	CCanvas *    oldcanvas;
	CBitmap *    background;
	UI_GADGET *     gadget;
	UI_GADGET *     keyboard_focus_gadget;
	struct ui_window * next;
	struct ui_window * prev;
} UI_WINDOW;

typedef struct  {
	int16_t           new_dx, new_dy;
	int16_t           new_buttons;
	int16_t           x, y;
	int16_t           dx, dy;
	int16_t           hidden;
	int16_t           backwards;
	int16_t           b1_status;
	int16_t           b1_last_status;
	int16_t           b2_status;
	int16_t           b2_last_status;
	int16_t           b3_status;
	int16_t           b3_last_status;
	int16_t           bg_x, bg_y;
	int16_t           bg_saved;
	CBitmap *    background;
	CBitmap *    pointer;
	uint32_t    time_lastpressed;
	int16_t           moved;
} UI_MOUSE;

#define BUTTON_PRESSED          1
#define BUTTON_RELEASED         2
#define BUTTON_JUST_PRESSED     4
#define BUTTON_JUST_RELEASED    8
#define BUTTON_DOUBLE_CLICKED   16

#define B1_PRESSED          (Mouse.b1_status & BUTTON_PRESSED)
#define B1_RELEASED         (Mouse.b1_status & BUTTON_RELEASED)
#define B1_JUST_PRESSED     (Mouse.b1_status & BUTTON_JUST_PRESSED)
#define B1_JUST_RELEASED    (Mouse.b1_status & BUTTON_JUST_RELEASED)
#define B1_DOUBLE_CLICKED   (Mouse.b1_status & BUTTON_DOUBLE_CLICKED)

extern CFont * ui_small_font;

extern UI_MOUSE Mouse;
extern UI_WINDOW * CurWindow;
extern UI_WINDOW * FirstWindow;
extern UI_WINDOW * LastWindow;

extern uint8_t CBLACK,CGREY,CWHITE,CBRIGHT,CRED;
extern UI_GADGET * selected_gadget;
extern int32_t last_keypress;

extern void Hline(int16_t x1, int16_t x2, int16_t y );
extern void Vline(int16_t y1, int16_t y2, int16_t x );
extern void ui_string_centered( int16_t x, int16_t y, char * s );
extern void ui_draw_box_out( int16_t x1, int16_t y1, int16_t x2, int16_t y2 );
extern void ui_draw_box_in( int16_t x1, int16_t y1, int16_t x2, int16_t y2 );
extern void ui_draw_line_in( int16_t x1, int16_t y1, int16_t x2, int16_t y2 );


void ui_init();
void ui_close();
int32_t InfoBox( int16_t x, int16_t y, int32_t NumButtons, char * text, ... );
void ui_string_centered( int16_t x, int16_t y, char * s );
int32_t PopupMenu( int32_t NumItems, char * text[] );

extern void ui_mouse_init();
extern CBitmap * ui_mouse_set_pointer( CBitmap * new );

extern void ui_mouse_process();
extern void ui_mouse_hide();
extern void ui_mouse_show();

#define WIN_BORDER 1
#define WIN_FILLED 2
#define WIN_SAVE_BG 4
#define WIN_DIALOG (4+2+1)

extern UI_WINDOW * ui_open_window( int16_t x, int16_t y, int16_t w, int16_t h, int32_t flags );
extern void ui_close_window( UI_WINDOW * wnd );

extern UI_GADGET * ui_gadget_add( UI_WINDOW * wnd, int16_t kind, int16_t x1, int16_t y1, int16_t x2, int16_t y2 );
extern UI_GADGET_BUTTON * ui_add_gadget_button( UI_WINDOW * wnd, int16_t x, int16_t y, int16_t w, int16_t h, char * text, int32_t (*function_to_call)(void) );
extern void ui_gadget_delete_all( UI_WINDOW * wnd );
extern void ui_window_do_gadgets( UI_WINDOW * wnd );
extern void ui_draw_button( UI_GADGET_BUTTON * button );

extern int32_t ui_mouse_on_gadget( UI_GADGET * gadget );

extern void ui_button_do( UI_GADGET_BUTTON * button, int32_t keypress );

extern void ui_listbox_do( UI_GADGET_LISTBOX * listbox, int32_t keypress );
extern void ui_draw_listbox( UI_GADGET_LISTBOX * listbox );
extern UI_GADGET_LISTBOX * ui_add_gadget_listbox( UI_WINDOW * wnd, int16_t x, int16_t y, int16_t w, int16_t h, int16_t numitems, char * list, int32_t text_width );

extern void ui_mega_process();

extern void ui_get_button_size( char * text, int32_t * width, int32_t * height );

extern UI_GADGET_SCROLLBAR * ui_add_gadget_scrollbar( UI_WINDOW * wnd, int16_t x, int16_t y, int16_t w, int16_t h, int32_t start, int32_t stop, int32_t position, int32_t window_size  );
extern void ui_scrollbar_do( UI_GADGET_SCROLLBAR * scrollbar, int32_t keypress );
extern void ui_draw_scrollbar( UI_GADGET_SCROLLBAR * scrollbar );


extern void ui_wprintf( UI_WINDOW * wnd, char * format, ... );
extern void ui_wprintf_at( UI_WINDOW * wnd, int16_t x, int16_t y, char * format, ... );

extern void ui_draw_radio( UI_GADGET_RADIO * radio );
extern UI_GADGET_RADIO * ui_add_gadget_radio( UI_WINDOW * wnd, int16_t x, int16_t y, int16_t w, int16_t h, int16_t group, char * text );
extern void ui_radio_do( UI_GADGET_RADIO * radio, int32_t keypress );

extern void ui_draw_checkbox( UI_GADGET_CHECKBOX * checkbox );
extern UI_GADGET_CHECKBOX * ui_add_gadget_checkbox( UI_WINDOW * wnd, int16_t x, int16_t y, int16_t w, int16_t h, int16_t group, char * text );
extern void ui_checkbox_do( UI_GADGET_CHECKBOX * checkbox, int32_t keypress );

extern UI_GADGET * ui_gadget_get_prev( UI_GADGET * gadget );
extern UI_GADGET * ui_gadget_get_next( UI_GADGET * gadget );
extern void ui_gadget_calc_keys( UI_WINDOW * wnd);

extern void ui_listbox_change( UI_WINDOW * wnd, UI_GADGET_LISTBOX * listbox, int16_t numitems, char * list, int32_t text_width );


extern void ui_draw_inputbox( UI_GADGET_INPUTBOX * inputbox );
extern UI_GADGET_INPUTBOX * ui_add_gadget_inputbox( UI_WINDOW * wnd, int16_t x, int16_t y, int16_t w, int16_t h, char * text );
extern void ui_inputbox_do( UI_GADGET_INPUTBOX * inputbox, int32_t keypress );


extern void ui_userbox_do( UI_GADGET_USERBOX * userbox, int32_t keypress );
extern UI_GADGET_USERBOX * ui_add_gadget_userbox( UI_WINDOW * wnd, int16_t x, int16_t y, int16_t w, int16_t h );
extern void ui_draw_userbox( UI_GADGET_USERBOX * userbox );


extern int32_t MenuX( int32_t x, int32_t y, int32_t NumButtons, char * text[] );

// Changes to a drive if valid.. 1=A, 2=B, etc
// If flag, then changes to it.
// Returns 0 if not-valid, 1 if valid.
int32_t file_chdrive( int32_t DriveNum, int32_t flag );

// Changes to directory in dir.  Even drive is changed.
// Returns 1 if failed.
//  0 = Changed ok.
//  1 = Invalid disk drive.
//  2 = Invalid directory.

int32_t file_chdir( char * dir );

int32_t file_getdirlist( int32_t MaxNum, char list[][13] );
int32_t file_getfilelist( int32_t MaxNum, char list[][13], char * filespec );
int32_t ui_get_filename( char * filename, char * Filespec, char * message  );


void * ui_malloc( int32_t size );
void ui_free( void * buffer );

UI_GADGET_KEYTRAP * ui_add_gadget_keytrap( UI_WINDOW * wnd, int32_t key_to_trap, int32_t (*function_to_call)(void)  );
void ui_keytrap_do( UI_GADGET_KEYTRAP * keytrap, int32_t keypress );

void ui_mouse_close();

#define UI_RECORD_MOUSE     1
#define UI_RECORD_KEYS      2
#define UI_STATUS_NORMAL    0
#define UI_STATUS_RECORDING 1
#define UI_STATUS_PLAYING   2
#define UI_STATUS_FASTPLAY  3

int32_t ui_record_events( int32_t NumberOfEvents, UI_EVENT * buffer, int32_t Flags );
int32_t ui_play_events_realtime( int32_t NumberOfEvents, UI_EVENT * buffer );
int32_t ui_play_events_fast( int32_t NumberOfEvents, UI_EVENT * buffer );
int32_t ui_recorder_status();
void ui_set_playback_speed( int32_t speed );

extern uint32_t ui_number_of_events;
extern uint32_t ui_eventCounter;


int32_t ui_get_file( char * filename, char * Filespec  );

int32_t MessageBoxN( int16_t xc, int16_t yc, int32_t NumButtons, char * text, char * Button[] );

void ui_draw_icon( UI_GADGET_ICON * icon );
void ui_icon_do( UI_GADGET_ICON * icon, int32_t keypress );
UI_GADGET_ICON * ui_add_gadget_icon( UI_WINDOW * wnd, char * text, int16_t x, int16_t y, int16_t w, int16_t h, int32_t k,int32_t (*f)(void) );

int32_t GetKeyCode(char * text);
int32_t DecodeKeyText( char * text );
void GetKeyDescription( char * text, int32_t keypress );

extern void menubar_init(char * filename );
extern void menubar_do( int32_t keypress );
extern void menubar_close();
extern void menubar_hide();
extern void menubar_show();

void ui_pad_init();
void ui_pad_close();
void ui_pad_activate( UI_WINDOW * wnd, int32_t x, int32_t y );
void ui_pad_deactivate();
void ui_pad_goto(int32_t n);
void ui_pad_goto_next();
void ui_pad_goto_prev();
void ui_pad_read( int32_t n, char * filename );
int32_t ui_pad_get_current();

void ui_barbox_open( char * text, int32_t length );
int32_t ui_barbox_update( int32_t position );
void ui_barbox_close();

void ui_reset_idleSeconds(void);
int32_t ui_get_idleSeconds(void);

extern char filename_list[300][13];
extern char directory_list[100][13];

extern int32_t ui_button_any_drawn;

#endif

