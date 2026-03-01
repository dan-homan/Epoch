#if FLTK_GUI

//-------------
// Change Log
//-------------
// 2011_12_11 -- some changes to add in a "knowledge_scale" parameter with 
//    appropriate modifications to search.cpp and score.cpp
//    Also worked on compiling this with the newest version of FLTK 1.3.0
//

////////////////////////////////////////////////////////////////
// fltk interface:
const char* copyright = 
"EXchess v7.92 (beta), Experimental Chess Program\n"
"Copyright (C) 1997-2016 Dan Homan   dchoman_at_gmail.com\n"
"\n"
"The GUI interface is based on the FLTK GUI toolkit Checkers example program:\n" 
"FLTK Checkers Copyright (C) 1997 Bill Spitzak    spitzak_at_d2.com\n"
"distributed under the GNU General Public License.\n"
"\n"
"Piece images come from the Xboard/Winboard distribution by Tim Mann:\n"
"http://www.tim-mann.org/xboard.html (now: http://www.gnu.org/software/xboard/)\n"
"distributed under the GNU General Public License and remain the copyright\n"
"of the orignal artist, Elmar Bartel.\n"
"\n"
"License:\n"
"----------------\n"
"This program is free software; you can redistribute it and/or modify "
"it under the terms of the GNU General Public License as published by "
"the Free Software Foundation; either version 3 of the License, or "
"(at your option) any later version.\n"
"\n"
"This program is distributed in the hope that it will be useful, "
"but WITHOUT ANY WARRANTY; without even the implied warranty of "
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU Library General Public "
"License along with this library; if not, see <http://www.gnu.org/licenses/>";

const char* ask_license = 
"EXchess v7.92 (beta), see \"copyright\" in  program menu for copyright info\n"
"\n"
"License:\n"
"----------------\n"
"This program is free software; you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation; either version 3 of the License, or\n"
"(at your option) any later version.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU Library General Public\n"
"License along with this library; if not, see <http://www.gnu.org/licenses/>\n"
"\nDo you agree to the terms of this license?\n";

const char* start_help =
"Click and Drag a piece to make a move.\n"
"\n"
"Hit the Spacebar for a menu of options. Shortcuts for menu\n" 
"items are listed on the right of the menu.\n"
"\n"
"EXchess defaults to 1 sec/move, 100 percent playing strength\n"
"and no thinking on opponents time (ponder next move off).\n"
"These settings can be changed in the menus\n"; 

#include <FL/x.H>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Bitmap.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Value_Slider.H>

#include "bitmaps/bitmaps.h"

int abortflag = 0;         // flag to abort search in FLTK
int FLTK_usebook = 1;      // flag to use book if available
int highlight_lastmove = 1; // flag to highlight last move made

char FLTK_startpos[256];   // string for starting position 
                           // -- could be regular or chess960
char FLTK_ms = 'w';
char FLTK_cast[5] = "KQkq";
char FLTK_ep[3] = "-";

int busy = 0;              // causes pop-up abort menu
int playing = 1;
int autoplay = 0;
int FLTK_post = 0;


Fl_Bitmap *bm[18][4][6];

void make_bitmaps() {
  for(int i=0; i<9; i++)
    for(int j=0; j<4; j++) 
      for(int k=0; k<6; k++)
	bm[i][j][k] = new Fl_Bitmap(builtInBits[i].bits[j][k], builtInBits[i].squareSize, builtInBits[i].squareSize);
}

static int erase_this = -1;  // real location of dragging piece, don't draw it
static int dragging = -1;	// piece being dragged
static int dragx;	// where it is
static int dragy;
static int showlegal;	// show legal moves

int bsizes[9][5] = {108,118, 9, 953,10,   
                     95,105, 8, 848, 9, 
                     80, 88, 7, 711, 8,
                     72, 80, 6, 644, 7,
                     64, 69, 5, 557, 6,
                     54, 60, 4, 484, 5,
		     45, 51, 3, 411, 4,
                     37, 43, 2, 346, 3,
		     29, 33, 1, 265, 2 };

// initial board/piece sizes
static int SZ=4;
static int ISIZE=64;
static int BOXSIZE=69;
static int BORDER=5;
static int BOARDSIZE=557; // (8*BOXSIZE+BORDER)
static int BMOFFSET=6;


int squarex(int i, int flip) { 
  if(flip) return (7^(i&7))*BOXSIZE+BMOFFSET; 
  else return (i&7)*BOXSIZE+BMOFFSET;
}
int squarey(int i, int flip) {
  if(flip) return (7^(7 - (i>>3)))*BOXSIZE+BMOFFSET; 
  else return (7 - (i>>3))*BOXSIZE+BMOFFSET;
}

void draw_piece(int sqr, int flip) {
  int x = squarex(sqr, flip);
  int y = squarey(sqr, flip);
  int pcolor = PSIDE(game.pos.sq[sqr]);
  int ptype = PTYPE(game.pos.sq[sqr])-1;
  if(ptype < 0 || pcolor < 0) return;
  if (!fl_not_clipped(x,y,ISIZE,ISIZE)) return;
  if(pcolor == BLACK) {
   fl_color(FL_WHITE); bm[SZ][0][ptype]->draw(x, y);
   fl_color(FL_BLACK); bm[SZ][1][ptype]->draw(x, y);   
  } else {
   fl_color(FL_WHITE); bm[SZ][2][ptype]->draw(x, y);
   fl_color(FL_BLACK); bm[SZ][3][ptype]->draw(x, y);
  }
}

//----------------------------------------------------------------

class Board : public Fl_Double_Window {
  void draw();
  int handle(int);
public:
  int dragging_from;
  int draggingx;
  int draggingy;
  int call_computer_move_again;  
  int flip;           // flag to flip board orientation

  void drag_piece(int, int, int);
  int drop_piece(int);
  void computer_move(int);
  Board(int w, int h) : Fl_Double_Window(w,h) { };
};

void Board::draw() {
  fl_draw_box(box(),0,0,w(),h(),color());
  fl_color(195,195,195);
  //fl_color(FL_GRAY_RAMP+15);
  int x;
  for (x=0; x<8; x++) for (int y=0; y<8; y++) {
    if (!((x^y)&1)) fl_rectf(BORDER+x*BOXSIZE, BORDER+y*BOXSIZE,
			     BOXSIZE-BORDER, BOXSIZE-BORDER);
  }
  fl_color(145,145,145);
  //fl_color(FL_GRAY_RAMP+9);
  for (x=0; x<8; x++) for (int y=0; y<8; y++) {
    if (((x^y)&1)) fl_rectf(BORDER+x*BOXSIZE, BORDER+y*BOXSIZE,
			     BOXSIZE-BORDER, BOXSIZE-BORDER);
  }
  fl_color(65,65,65);
  //fl_color(FL_GRAY_RAMP+4);
  for (x=0; x<9; x++) {
    fl_rectf(x*BOXSIZE,0,BORDER,h());
    fl_rectf(0,x*BOXSIZE,w(),BORDER);
  }
  for (int j = 0; j < 64; j++) if (j != dragging_from) {
    draw_piece(j, flip);
  }
  if(dragging_from > -1) {
    int pcolor = PSIDE(game.pos.sq[dragging_from]);
    int ptype = PTYPE(game.pos.sq[dragging_from])-1;
    if(ptype < 0 || pcolor < 0) return;
    if (!fl_not_clipped(draggingx,draggingy,ISIZE,ISIZE)) return;
    if(pcolor == BLACK) {
      fl_color(FL_WHITE); bm[SZ][0][ptype]->draw(draggingx, draggingy);
      fl_color(FL_BLACK); bm[SZ][1][ptype]->draw(draggingx, draggingy);   
    } else {
      fl_color(FL_WHITE); bm[SZ][2][ptype]->draw(draggingx, draggingy);
      fl_color(FL_BLACK); bm[SZ][3][ptype]->draw(draggingx, draggingy);
    }
  }
  if(game.pos.last.t && highlight_lastmove) {
    int from_x = FILE(game.pos.last.b.from);
    int from_y = RANK(game.pos.last.b.from);
    int to_x = FILE(game.pos.last.b.to);
    int to_y = RANK(game.pos.last.b.to);

    if(!flip) { from_y = 7-from_y; to_y = 7-to_y; }
    else { from_x = 7-from_x; to_x = 7-to_x; }

    fl_rect(BORDER+from_x*BOXSIZE, BORDER+from_y*BOXSIZE,
	    BOXSIZE-BORDER, BOXSIZE-BORDER, FL_YELLOW);
    fl_rect(BORDER+from_x*BOXSIZE+1, BORDER+from_y*BOXSIZE+1,
	    BOXSIZE-BORDER-2, BOXSIZE-BORDER-2, FL_YELLOW);
    fl_rect(BORDER+to_x*BOXSIZE, BORDER+to_y*BOXSIZE,
	    BOXSIZE-BORDER, BOXSIZE-BORDER, FL_YELLOW);
    fl_rect(BORDER+to_x*BOXSIZE+1, BORDER+to_y*BOXSIZE+1,
	    BOXSIZE-BORDER-2, BOXSIZE-BORDER-2, FL_YELLOW);
  }
}

// drag the piece on square i to dx dy, or undo drag if i is zero
void Board::drag_piece(int j, int dx, int dy) {
  dragging_from = j;
  draggingx = dx;
  draggingy = dy;
  damage(FL_DAMAGE_ALL, dragx, dragy, ISIZE, ISIZE);
  damage(FL_DAMAGE_ALL, dx, dy, ISIZE, ISIZE);
  dragx = dx;
  dragy = dy;  
}

// drop currently dragged piece on square "dragging_to"
//  == returns 1 if move is legal (0 otherwise)
int Board::drop_piece(int dragging_to) {  
  char outstring[200], mstring[10];
  // identify a Chess960 castle by having king capture rook and
  //  replace the move with "O-O" or "O-O-O" as appropriate
  if(PSIDE(game.pos.sq[dragging_from]) == game.pos.wtm && 
     PTYPE(game.pos.sq[dragging_from]) == KING &&
     PSIDE(game.pos.sq[dragging_to]) == game.pos.wtm &&
     PTYPE(game.pos.sq[dragging_to]) == ROOK &&
     RANK(dragging_from) == RANK(dragging_to)) {
    if(dragging_from > dragging_to) snprintf(mstring, sizeof(mstring), "O-O-O");
    else snprintf(mstring, sizeof(mstring), "O-O");
  } else {
    snprintf(mstring, sizeof(mstring), "%c%i%c%i", char(FILE(dragging_from) + 97),(RANK(dragging_from) + 1),
                               char(FILE(dragging_to) + 97),(RANK(dragging_to) + 1));
  }
  game.best = game.pos.parse_move(mstring, &game.ts.tdata[0]);
  if(game.best.t && !game.over) { 
    // make my move
    game.p_side = game.pos.wtm;
    make_move(); game.T++; 
    game.pos.allmoves(&game.movelist, &game.ts.tdata[0]);     // find legal moves
    dragging_from = -1; redraw();
    Fl::flush();
    if(game.over) {
      snprintf(outstring, sizeof(outstring), "EXchess -- Last Move %s, Game Over: %s",
	      game.lmove, game.overstring);
      label(outstring);
    }
    return 1;
  } else {
    dragging_from = -1;
    redraw(); 
    Fl::flush();
    return 0;
  }
}

void Board::computer_move(int help) {
  char outstring[200], side_to_move[2][20] = { "Black-to-Move", "White-to-Move" }; 
  // check if we are pondering (shouldn't happen, but just in case)
  if (game.ts.ponder) { 
    call_computer_move_again = 1;
    abortflag = 1;
    write_out("Ponder Error in FLTK GUI!\n"); 
    return; 
  }
  if (game.over) return;
  // be sure some control variables are set appropriately
  abortflag = 0;
  call_computer_move_again = 0; 
  cursor(FL_CURSOR_WAIT);
  Fl::flush();
  busy = 1; 
  // Write out some information about search
  write_out("Thinking ...\n");
  // Change status bar to reflect the on-going search
  snprintf(outstring, sizeof(outstring), "EXchess -%s- Last Move: %s, Computer Thinking...",
	  side_to_move[game.pos.wtm], game.lmove);
  label(outstring);
  // set the player side to be opposite the side on move
  game.p_side = game.pos.wtm^1;
  // call the make_move() function from main.cpp which does the search
  //   and executes the move
  make_move();
  // change the status bar to reflect the results of the search
  snprintf(outstring, sizeof(outstring), "EXchess -%s- Last Move: %s",
	  side_to_move[game.pos.wtm], game.lmove);
  label(outstring);
  // draw the new board
  redraw();
  // put check or game-over info in the status bar
  if(game.over) {
    snprintf(outstring, sizeof(outstring), "EXchess -- Last Move %s, Game Over: %s",
	    game.lmove, game.overstring);
    label(outstring);
  } else if(game.pos.in_check() && !autoplay) {
    snprintf(outstring, sizeof(outstring), "EXchess -%s- Last Move: %s, Check!",
	    side_to_move[game.pos.wtm], game.lmove);
    label(outstring);
  }
  // clean up from the move and get ready for the next move
  game.ts.last_ponder = 0;
  game.T++;
  game.pos.allmoves(&game.movelist, &game.ts.tdata[0]);     // find legal moves
  busy = 0;
  cursor(FL_CURSOR_DEFAULT);
  // now ponder next move if appropriate
  if(!game.both && !game.ts.last_ponder && ponder_flag && !game.over) {
    snprintf(outstring, sizeof(outstring), "EXchess -%s- Last Move: %s (Pondering Next Move)",
	    side_to_move[game.pos.wtm], game.lmove);
    label(outstring);
    game.ts.ponder = 1;
    game.ts.search(game.pos, 1, game.T+1, &game);
    game.ts.last_ponder = 1;
    game.ts.ponder = 0;
    snprintf(outstring, sizeof(outstring), "EXchess -%s- Last Move: %s",
	    side_to_move[game.pos.wtm], game.lmove);
    label(outstring);
    // if there was a ponder race while we were pondering (see above and in
    //     Board::handle function) then have the computer make a move when 
    //     pondering is done...  
    if(call_computer_move_again) {
      call_computer_move_again = 0; 
      computer_move(0);
    }
  }
}

extern Fl_Menu_Item menu[];
extern Fl_Menu_Item time_menu[];
extern Fl_Menu_Item strength_menu[];
extern Fl_Menu_Item busymenu[];
extern Fl_Menu_Item pondermenu[];

int Board::handle(int e) {
  int legal;
  if (busy) {
    const Fl_Menu_Item* m;
    switch(e) {
    case FL_PUSH:
      m = busymenu->popup(Fl::event_x(), Fl::event_y(), 0, 0, 0);
      if (m) m->do_callback(this, (void*)m);
      return 1;
    case FL_SHORTCUT:
      //
      // test the available shortcuts
      //
      m = busymenu->test_shortcut();
      if (m) {m->do_callback(this, (void*)m); return 1;}
      //
      // if the spacebar is hit, pull up the full menu
      // 
      if(Fl::event_key() == 32) {
	m = busymenu->popup(ISIZE, ISIZE, 0, 0, 0);	
	if (m) {m->do_callback(this, (void*)m); return 1;}
      }
      return 0;
    default:
      return 0;
    }
  }
  move *t, *n;
  static int deltax, deltay;
  int dist;
  const Fl_Menu_Item* m;
  switch (e) {
  case FL_PUSH:
    if (Fl::event_button() > 1) {
      if(game.ts.ponder) 
        m = pondermenu->popup(ISIZE, ISIZE, 0, 0, 0);
      else
        m = menu->popup(ISIZE, ISIZE, 0, 0, 0);	
      if (m) { m->do_callback(this, (void*)m); return 1; }
    }
    if (!game.over) {
      for (int j = 0; j < 64; j++) if(game.pos.sq[j]) {
        int x = squarex(j,flip);
        int y = squarey(j,flip);
        if (Fl::event_inside(x,y,BOXSIZE,BOXSIZE)) {
          deltax = Fl::event_x()-x;
          deltay = Fl::event_y()-y;
          drag_piece(j,x,y);
          // stop pondering
          //if(game.ts.ponder) { abortflag=1; }
          return 1;
        }
      }
    }
    return 0;
  case FL_SHORTCUT:
    if(game.ts.ponder) 
      m = pondermenu->test_shortcut();
    else 
      m = menu->test_shortcut();
    if (m) {m->do_callback(this, (void*)m); return 1;}
    //
    // if the spacebar is hit, pull up the full menu
    // 
    if(Fl::event_key() == 32) {
      if(game.ts.ponder) 
	m = pondermenu->popup(ISIZE, ISIZE, 0, 0, 0);
      else
	m = menu->popup(ISIZE, ISIZE, 0, 0, 0);	
      if (m) {m->do_callback(this, (void*)m); return 1;}
    }
    return 0;
  case FL_DRAG:
    if(!game.over) drag_piece(dragging_from, Fl::event_x()-deltax, Fl::event_y()-deltay);
    return 1;
  case FL_RELEASE:
    legal = 0;
    if(!game.over) for (int j = 0; j < 64; j++) {
      int x = squarex(j,flip);
      int y = squarey(j,flip);
      if (Fl::event_inside(x,y,BOXSIZE,BOXSIZE)) {
	legal = drop_piece(j);
      }
    }
    // if that was a legal move, now it is the computer's turn
    //    -- if not pondering call computer move directly, otherwise
    //       abort pondering and tell function to call the computer 
    //       once the pondering is complete
    if(legal) { 
      if(!game.ts.ponder) computer_move(0);
      else { call_computer_move_again = 1; abortflag = 1; }
    }
    return 1;
  default:
    return 0;
  }
}

void quit_cb(Fl_Widget*, void*) {exit(0);}

int FLTKmain(int argc, char** argv) {
  Fl::visual(FL_DOUBLE|FL_INDEX);
  int yes = 0;
  fstream check_file;
  char agree_file[FILENAME_MAX];
  strcpy(agree_file, exec_path);
  strcat(agree_file, "agree_license.txt");
  check_file.open(agree_file, IOS_IN);
  if(check_file.is_open()) yes = 1;  
  if(!yes) {  
    yes = fl_ask(ask_license,NULL);
    if(yes)  { 
      ofstream ask_file;
      ask_file.open(agree_file, IOS_OUT);
    }
  }
  if(yes) {
    // set a few GUI defaults...
    //  -- ponder off and search time = 1 sec/move
    ponder_flag = 0;
    game.mttc = 0; game.base = 1; game.inc = 0; 
    game.omttc = game.mttc; game.timeleft[0] = game.base*100; game.timeleft[1] = game.base*100;     
    // put a copy of the starting position in FLTK_startpos 
    //  for exporting to PGN later
    strcpy(FLTK_startpos, i_pos);
    // startup the game...
    fl_message(start_help,NULL);
    Board b(BOARDSIZE,BOARDSIZE);
    make_bitmaps();
    b.dragging_from = -1;
    b.call_computer_move_again = 0;
    b.flip = 0;
    b.label("EXchess");
    b.callback(quit_cb);
    b.show(argc,argv);
    return Fl::run();
  } else return 0;
} 

void highlight_cb(Fl_Widget*b, void*v) {
  highlight_lastmove = !highlight_lastmove;
  ((Fl_Menu_Item*)v)->flags =
    highlight_lastmove ? FL_MENU_TOGGLE|FL_MENU_VALUE : FL_MENU_TOGGLE;
}

void ponder_cb(Fl_Widget*b, void*v) {
  ponder_flag = !ponder_flag;
  ((Fl_Menu_Item*)v)->flags =
    ponder_flag ? FL_MENU_TOGGLE|FL_MENU_VALUE : FL_MENU_TOGGLE;
}

void book_cb(Fl_Widget*b, void*v) {
  FLTK_usebook = !FLTK_usebook;
  game.book = FLTK_usebook;
  ((Fl_Menu_Item*)v)->flags =
    FLTK_usebook ? FL_MENU_TOGGLE|FL_MENU_VALUE : FL_MENU_TOGGLE;
}

void stopponder_cb(Fl_Widget*b, void*v) {
  if(game.ts.ponder) abortflag = 1;
}

void autoplay_cb(Fl_Widget*bp, void*) {
  if (autoplay || busy) {autoplay = 0; return;}
  Board* b = (Board*)bp;
  autoplay = 1;
  game.both = 0; 
  game.ts.analysis_mode = 0;
  // prevent pondering
  if(ponder_flag) ponder_flag = 0;
  // loop playing moves   
  while(autoplay && !game.over) {  
    b->computer_move(0); 
  }
}

#include <FL/Fl_Box.H>
Fl_Window *copyright_window;
void copyright_cb(Fl_Widget*, void*) {
  if (!copyright_window) {
    copyright_window = new Fl_Window(550,380,"Copyright & License");
    copyright_window->color(FL_WHITE);
    Fl_Box *b = new Fl_Box(10,0,530,380,copyright);
    b->labelsize(12);
    b->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_WRAP);
    copyright_window->end();
  }
  copyright_window->hotspot(copyright_window);
  copyright_window->set_non_modal();
  copyright_window->show();
}

#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Display.H>
Fl_Window *output_win;
Fl_Text_Buffer *searchout_buffer = 0;
Fl_Text_Display *searchout;
void post_cb(Fl_Widget*, void*) {
  FLTK_post = 1;
  if(!output_win) {
    output_win = new Fl_Window(600,300,"Search Output");
    searchout_buffer = new Fl_Text_Buffer;
    searchout = new Fl_Text_Display(5,5,590,290,"");
    searchout->buffer(searchout_buffer);
    output_win->resizable(searchout);
  }
  searchout_buffer->append("Thinking...\n");
  output_win->hotspot(searchout);
  output_win->show();
}

void hint_cb(Fl_Widget*pb, void*) {
  move hint;
  char mstring[10];
  hint.t = 0;
  if(game.ts.last_ponder) hint = game.ts.ponder_move;
  else if(game.book) hint = opening_book(game.pos.hcode, game.pos, &game);
  if(!hint.t) hint = game.ts.tdata[0].pc[0][1];
  if(hint.t) {
    game.pos.print_move(hint, mstring,&game.ts.tdata[0]);
    fl_message("Hint Move: %s", mstring);
  } else { fl_message("No Hint Available"); }
}

void newgame_cb(Fl_Widget*b, void*) {
  game.p_side = 1; 
  if(!game.ts.analysis_mode) 
    { game.both = 0; game.book = FLTK_usebook; game.learn_bk = BOOK_LEARNING; }
  game.T = 1; game.mttc = game.omttc; 
  game.learn_count = 0; game.learned = 0; game.ts.no_book = 0;
  game.over = 0; game.setboard(i_pos, 'w', "KQkq", "-"); 
  game.timeleft[0] = game.base*100;
  game.timeleft[1] = game.base*100;
  autoplay = 0; abortflag = 0;  
  b->redraw();
  // put a copy of the starting position in FLTK_startpos 
  //  for exporting to PGN later
  strcpy(FLTK_startpos, i_pos);
}

void newgame960_cb(Fl_Widget*b, void*) {
  game.p_side = 1; 
  //----------------------------------
  // select start position at random
  //----------------------------------
  char s_pos[256] = "xxxxxxxx/pppppppp/8/8/8/8/PPPPPPPP/xxxxxxxx";
  int bishop_file1 = (rand()%4)*2;
  s_pos[bishop_file1+35] = 'B';
  s_pos[bishop_file1] = 'b';
  int bishop_file2 = (rand()%4)*2+1;
  s_pos[bishop_file2+35] = 'B';
  s_pos[bishop_file2] = 'b';
  int queen_file = (rand()%6);
  int unoccupied_count = 0;
  for(int i = 0; i<8; i++) {
    if(s_pos[i] == 'x') {
      unoccupied_count++;
      if(unoccupied_count > queen_file) {
	s_pos[i+35] = 'Q';
	s_pos[i] = 'q';
	i = 8;
      }
    }
  }
  int knight_file1 = (rand()%5);
  unoccupied_count = 0;
  for(int i = 0; i<8; i++) {
    if(s_pos[i] == 'x') {
      unoccupied_count++;
      if(unoccupied_count > knight_file1) {
	s_pos[i+35] = 'N';
	s_pos[i] = 'n';
	i = 8;
      }
    }
  }
  int knight_file2 = (rand()%4);
  unoccupied_count = 0;
  for(int i = 0; i<8; i++) {
    if(s_pos[i] == 'x') {
      unoccupied_count++;
      if(unoccupied_count > knight_file2) {
	s_pos[i+35] = 'N';
	s_pos[i] = 'n';
	i = 8;
      }
    }
  }
  // place the remaining pieces
  unoccupied_count = 0;
  for(int i = 0; i<8; i++) {
    if(s_pos[i] == 'x') {
      unoccupied_count++;
      if(unoccupied_count == 1) {
	s_pos[i+35] = 'R';
	s_pos[i] = 'r';
      }
      if(unoccupied_count == 2) {
	s_pos[i+35] = 'K';
	s_pos[i] = 'k';
      }
      if(unoccupied_count == 3) {
	s_pos[i+35] = 'R';
	s_pos[i] = 'r';
	i = 8;
      }
    }
  }
    
  //-------------------
  // setup the game
  //-------------------
  if(!game.ts.analysis_mode) 
    { game.both = 0; game.book = FLTK_usebook; game.learn_bk = BOOK_LEARNING; }
  game.T = 1; game.mttc = game.omttc; 
  game.learn_count = 0; game.learned = 0; game.ts.no_book = 0;
  game.over = 0; game.setboard(s_pos, 'w', "KQkq", "-"); 
  game.timeleft[0] = game.base*100;
  game.timeleft[1] = game.base*100;
  autoplay = 0; abortflag = 0;  
  b->redraw();
  fl_message("Chess960 (Fischer Random Chess) Game Started\nCastle by 'capturing' the castling rook with king.");

  // put a copy of the starting position in FLTK_startpos 
  //  for exporting to PGN later
  strcpy(FLTK_startpos, s_pos);

}

void savegame_cb(Fl_Widget*b, void*) {
  char *savefilename; 
  ofstream savefile;
  char game_savename[FILENAME_MAX];
  strcpy(game_savename, exec_path);
  strcat(game_savename, "game.sgf");
  savefilename = fl_file_chooser("Save Game", "*.sgf", game_savename);  
  savefile.open(savefilename, IOS_OUT);
  if(savefile.is_open()) {
    savefile.write((char *) &game, sizeof(game_rec));
  }
}

void loadgame_cb(Fl_Widget*bp, void*) {
  char outstring[200], side_to_move[2][20] = { "Black-to-Move", "White-to-Move" }; 
  char *loadfilename; 
  Board* b = (Board*)bp;
  fstream loadfile;
  loadfilename = fl_file_chooser("Load Game", "*.sgf", exec_path);  
  loadfile.open(loadfilename, IOS_IN);
  if(loadfile.is_open()) {
    loadfile.read((char *) &game, sizeof(game_rec));
  }
  autoplay = 0; abortflag = 0;
  snprintf(outstring, sizeof(outstring), "EXchess -%s- Last Move: %s",
	  side_to_move[game.pos.wtm], game.lmove);
  b->label(outstring);
  b->redraw();
}

void exportpgn_cb(Fl_Widget*b, void*) {
  int TURN; TURN = game.T;
  char *gname;
  char game_pgnname[FILENAME_MAX];
  strcpy(game_pgnname, exec_path);
  strcat(game_pgnname, "game.pgn");
  gname = fl_file_chooser("Export PGN", "*.pgn", game_pgnname);

  char resp, mstring[10];
  char Event[30], White[30], Black[30], Date[30], result[30];

  strcpy(Event, "Chess Match");
  strcpy(Date, "??.??.????");
  if (game.p_side)
    { strcpy(White, "Human"); strcpy(Black, "EXchess"); }
  else
    { strcpy(White, "EXchess"); strcpy(Black, "Human"); }

  ofstream outfile(gname);

  outfile <<   "[Event: " << Event << " ]";
  outfile << "\n[Date: " << Date << " ]";
  outfile << "\n[White: " << White << " ]";
  outfile << "\n[Black: " << Black << " ]";
  outfile << "\n[Startpos: " << FLTK_startpos 
	              << " " << FLTK_ms 
	  << " " << FLTK_cast << " " << FLTK_ep << " ]";

  // set the result string
  if(game.over)
    { strcpy(result, game.overstring); }
  else strcpy(result, " adjourned");

  outfile << "\n[Result: " << result << " ]\n\n";

  // set the board up from the starting position
  game.setboard(FLTK_startpos, FLTK_ms, FLTK_cast, FLTK_ep);

  // play through the game and record the moves in a file
  for(int i = 1; i < TURN; i++)
   {
     game.pos.print_move(game.game_history[i-1], mstring, &game.ts.tdata[0]);
    if (game.pos.wtm) outfile << (int((double)i/2) + 1) << ". " << mstring;
    else outfile << mstring;
    outfile << " ";
    if(!(game.T%12)) outfile << "\n";
    game.pos.exec_move(game.game_history[i-1], 0);
    game.T++;
   }

  outfile << result;  outfile << "\n";

}

void flip_cb(Fl_Widget*pb, void*) {
  ((Board*)pb)->flip=(((Board*)pb)->flip)^1;
  ((Board*)pb)->redraw();
}

void switch_cb(Fl_Widget*pb, void*) {
  game.both = 0; 
  game.ts.analysis_mode = 0;
  if(game.over) return;
  // stop pondering
  if(game.ts.ponder) { 
    // after ponder finishes, set flag to do another search
    ((Board*)pb)->call_computer_move_again = 1;
    // abort ponder
    abortflag = 1; 
  }  else {
    ((Board*)pb)->computer_move(0);
  }
  if(game.pos.wtm == WHITE) { fl_message("Computer plays black"); }
  else { fl_message("Computer plays white"); }
}

void move_cb(Fl_Widget*pb, void*) {
  switch_cb(pb, NULL);
}

void strength_cb(Fl_Widget*pb, void*) {
  const char *inp_str;
  int f1;
  
  //char *curr_knowledge;
  //sprintf(curr_knowledge, "%i", game.knowledge_scale);

  inp_str = fl_input("Enter percent of chess skill used (1 to 100), current value = %i",
                    "", game.knowledge_scale); 

  if(inp_str == NULL) return;

  sscanf(inp_str, "%i", &f1);
  if(f1 >= 1 && f1 <= 100) {
    game.knowledge_scale = f1;
    fl_message("Chess skill used = %i percent", game.knowledge_scale);
  } else {
    fl_message("Must enter a value between 1 and 100, using %i percent", game.knowledge_scale);
  }
}

void threads_cb(Fl_Widget*pb, void*) {
  const char *inp_str;
  int f1;
  
  //char *curr_knowledge;
  //sprintf(curr_knowledge, "%i", game.knowledge_scale);

  inp_str = fl_input("Enter number of processors to use (1 to 4), current value = %i\nDo not exceed the maximum processors of your computer.",
                    "", THREADS); 

  if(inp_str == NULL) return;

  sscanf(inp_str, "%i", &f1);
  if(f1 >= 1 && f1 <= 4) {
    THREADS = f1;
    fl_message("Number of Processors Used = %i", THREADS);
    game.ts.initialize_extra_threads();
  } else {
    fl_message("Must enter a value between 1 and 4, using %i processors", THREADS);
  }
}

void time_cb(Fl_Widget*pb, void*) {
   const Fl_Menu_Item* m;
   m = time_menu->popup(Fl::event_x(), Fl::event_y(), 0, 0, 0);
   if (m) m->do_callback((Board*)pb, (void*)m);
}
void time1_cb(Fl_Widget*, void*) {
  game.mttc = 0; game.base = 1; game.inc = 0; 
  game.omttc = game.mttc; game.timeleft[0] = game.base*100; game.timeleft[1] = game.base*100; 
}
void time2_cb(Fl_Widget*, void*) {
  game.mttc = 0; game.base = 2; game.inc = 0;
  game.omttc = game.mttc; game.timeleft[0] = game.base*100; game.timeleft[1] = game.base*100; 
}
void time5_cb(Fl_Widget*, void*) {
  game.mttc = 0; game.base = 5; game.inc = 0;
  game.omttc = game.mttc; game.timeleft[0] = game.base*100; game.timeleft[1] = game.base*100; 
}
void time10_cb(Fl_Widget*, void*) {
  game.mttc = 0; game.base = 10; game.inc = 0;
  game.omttc = game.mttc; game.timeleft[0] = game.base*100; game.timeleft[1] = game.base*100; 
}
void time30_cb(Fl_Widget*, void*) {
  game.mttc = 0; game.base = 30; game.inc = 0;
  game.omttc = game.mttc; game.timeleft[0] = game.base*100; game.timeleft[1] = game.base*100; 
}
void time60_cb(Fl_Widget*, void*) {
  game.mttc = 0; game.base = 60; game.inc = 0;
  game.omttc = game.mttc; game.timeleft[0] = game.base*100; game.timeleft[1] = game.base*100; 
}
void time90_cb(Fl_Widget*, void*) {
  game.mttc = 0; game.base = 90; game.inc = 0;
  game.omttc = game.mttc; game.timeleft[0] = game.base*100; game.timeleft[1] = game.base*100; 
}
void time120_cb(Fl_Widget*, void*) {
  game.mttc = 0; game.base = 120; game.inc = 0;
  game.omttc = game.mttc; game.timeleft[0] = game.base*100; game.timeleft[1] = game.base*100; 
}
void time180_cb(Fl_Widget*, void*) {
  game.mttc = 0; game.base = 180; game.inc = 0;
  game.omttc = game.mttc; game.timeleft[0] = game.base*100; game.timeleft[1] = game.base*100; 
}
void time_generic_cb(Fl_Widget*, void*) {
  const char *inp_str;
  float f1, f2, f3; f1 = -1; f2 = -1; f3 = -1;
  
  inp_str = fl_input("Please input the time control as 3 numbers in the format \'moves base increment\'\nAn example would be \'40 5 1\' for 40 moves in 5 minutes with a 1 second increment per move", ""); 

  if(inp_str == NULL) return;

  sscanf(inp_str, "%f %f %f", &f1, &f2, &f3);
  if(f1 > -1) game.mttc = int(f1);
  if(f2 > 0) game.base = f2*60.0;
  if(f3 > -1) game.inc = int(f3); 

  fl_message("New time control is %i moves in %3.1f minutes with an extra %i seconds per move", game.mttc, game.base/60.0, int(game.inc));

  game.omttc = game.mttc; 
  game.timeleft[0] = game.base*100; game.timeleft[1] = game.base*100; 
 
}

void setup_cb(Fl_Widget*pb, void*) {
  const char *inp_str;
  char s_pos[256], cast[5], ep[3], ms;
  
  inp_str = fl_input("Please input a EPD string in the format: \'position side_to_move castling_rights en_passant\'\nAn example would be \'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -\' for the starting position.", ""); 
  if(inp_str) sscanf(inp_str, "%s %c %s %s", s_pos, &ms, cast, ep);
  else return;

  // Save starting position and get strings in
  //  right format for setboard
  strcpy(FLTK_startpos, s_pos);
  strcpy(FLTK_cast, cast);
  strcpy(FLTK_ep, ep);
  FLTK_ms = ms;

  game.setboard(FLTK_startpos, FLTK_ms, FLTK_cast, FLTK_ep);
  Board* b = (Board*)pb;
  b->redraw(); 
}

void undo_cb(Fl_Widget*pb, void*) {
  char outstring[200], side_to_move[2][20] = { "Black-to-Move", "White-to-Move" }; 
  Board* b = (Board*)pb;
  if(game.ts.ponder) abortflag = 1;
  takeback(1);
  b->redraw(); b->label("EXchess");
  takeback(1);
  b->redraw(); b->label("EXchess");
  snprintf(outstring, sizeof(outstring), "EXchess -%s-",
	  side_to_move[game.pos.wtm]);
  b->label(outstring);
}

void shrink_cb(Fl_Widget*pb, void*) {
  Board* b = (Board*)pb;
  if(SZ < 8) {
        SZ++;
        ISIZE = bsizes[SZ][0];
        BOXSIZE = bsizes[SZ][1];
        BORDER = bsizes[SZ][2];        
        BOARDSIZE = bsizes[SZ][3];
        BMOFFSET = bsizes[SZ][4];
	b->size(BOARDSIZE,BOARDSIZE);
	b->damage(1);
  }
}

void grow_cb(Fl_Widget*pb, void*) {
  Board* b = (Board*)pb;
  if(SZ > 0) {
        SZ--;
        ISIZE = bsizes[SZ][0];
        BOXSIZE = bsizes[SZ][1];
        BORDER = bsizes[SZ][2];        
        BOARDSIZE = bsizes[SZ][3];
        BMOFFSET = bsizes[SZ][4];
	b->size(BOARDSIZE,BOARDSIZE);
	b->damage(1);
  }
}

void movenow_cb(Fl_Widget*, void*) {abortflag = 1;}

void continue_cb(Fl_Widget*, void*) {}

Fl_Menu_Item time_menu[] = {
  {"1 sec/move", 0, time1_cb},
  {"2 sec/move", 0, time2_cb},
  {"5 sec/move", 0, time5_cb},
  {"10 sec/move", 0, time10_cb},
  {"30 sec/move", 0, time30_cb},
  {"60 sec/move", 0, time60_cb},
  {"90 sec/move", 0, time90_cb},
  {"120 sec/move", 0, time60_cb},
  {"180 sec/move", 0, time180_cb},
  {"Generic Time Control", 0, time_generic_cb},
  {0}};

Fl_Menu_Item menu[] = {
  {"New Game", 0, newgame_cb},
  {"New Chess960 Game", 0, newgame960_cb},
  {"Save Game", 0, savegame_cb},
  {"Load Game", 0, loadgame_cb},
  {"Export PGN", 0, exportpgn_cb, 0, FL_MENU_DIVIDER},
  {"Autoplay", 'a', autoplay_cb},
  {"Move For Me", 'm', move_cb},
  {"Switch Sides", 's', switch_cb},
  {"Hint", 'h', hint_cb},
  {"Undo", 'u', undo_cb, 0, FL_MENU_DIVIDER},
  {"Setup Board", 's', setup_cb},
  {"Show Thinking", 'p', post_cb},
  {"Computer Thinking Time", 0, time_cb}, 
  {"Computer Strength", 0, strength_cb},
  {"Processors Used", 0, threads_cb, 0, FL_MENU_DIVIDER},
  {"Smaller Board", '-', shrink_cb},
  {"Larger Board", '+', grow_cb},
  {"Flip Board", 'f', flip_cb, 0, FL_MENU_DIVIDER},
  {"Ponder Next Move", 0, ponder_cb, 0, FL_MENU_TOGGLE},
  {"Highlight Last Move", 0, highlight_cb, 0, FL_MENU_TOGGLE|FL_MENU_VALUE},
  {"Use Opening Book", 0, book_cb, 0, FL_MENU_TOGGLE|FL_MENU_VALUE|FL_MENU_DIVIDER},
  {"Copyright/License", 'c', copyright_cb},
  {"Quit", 'q', quit_cb},
  {0}};

Fl_Menu_Item pondermenu[] = {
  {"Stop Pondering", 0, stopponder_cb},
  {"Move For Me", 'm', move_cb},
  {"Switch Sides", 's', switch_cb},
  {"Hint", 'h', hint_cb},
  {"Undo", 'u', undo_cb, 0, FL_MENU_DIVIDER},
  {"Smaller Board", '-', shrink_cb},
  {"Larger Board", '+', grow_cb},
  {"Flip Board", 'f', flip_cb, 0, FL_MENU_DIVIDER},
  {"Copyright/License", 'c', copyright_cb},
  {"Quit", 'q', quit_cb},
  {0}};

Fl_Menu_Item busymenu[] = {
  {"Move Now", '.', movenow_cb},
  {"Stop Autoplay", 'a', autoplay_cb},
  {"Continue", 0, continue_cb, 0, FL_MENU_DIVIDER},
  {"Copyright/License", 'c', copyright_cb},
  {"Quit", 'q', quit_cb},
  {0}};

#endif

