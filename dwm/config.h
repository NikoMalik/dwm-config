/* See LICENSE file for copyright and license details. */






/* appearance */
#include <X11/X.h>

//smart graps
static const unsigned int gappih    = 20;       /* horiz inner gap between windows */
static const unsigned int gappiv    = 10;       /* vert inner gap between windows */
static const unsigned int gappoh    = 10;       /* horiz outer gap between windows and screen edge */
static const unsigned int gappov    = 30;       /* vert outer gap between windows and screen edge */
static       int smartgaps          = 0;        /* 1 means no outer gap when there is only one window */



static const int user_bh = 2;


static const unsigned int borderpx  = 2;        /* border pixel of windows */
static const unsigned int snap      = 30;       /* snap pixel */
static const int showbar            = 1;        /* 0 means no bar */
static const int topbar             = 1;        /* 0 means bottom bar */
// static const int vertpad = 10 ; /* vertical padding of bar */
// static const int sidepad =10 ; /* horizontal padding of bar */
static const int vertpad = 0 ; /* vertical padding of bar */
static const int sidepad =0 ; /* horizontal padding of bar */

static const char *fonts[]          = {
	 "Maple Mono NF:size=12:style=Regular",
	 "Font Awesome 6 Free Solid:size=12",
	 "Noto Sans:size=12"

	  };
static const char dmenufont[]       = "Maple Mono NF:size=12:style=Regular";



// static const char col_background[]  = "#000000";  /* main background, Vintage background */
// static const char col_foreground[]  = "#e5e5e5";  /* main text, Vintage bright white */
// static const char col_highlight[]   = "#000000";  /* secondary background, same as main background */
// static const char col_accent[]      = "#778e61";  /* active text, same as main text (white) */
// static const char col_white_selection[] = "#778e61"; /* selection, Vintage ui.text.focus (active green) */
// static const char col_debkg[]       = "#778e61";  /* panel background, Vintage green */
// static const char col_acbkg[]       = "#000000";  /* active panel background, Vintage ui.text.focus */
// static const char col_acfor[]       = "#e5e5e5";  /* active text, same as main text (white) */
// static const char col_debor[]       = "#e5e5e5";  /* default border, Vintage green */
// static const char col_sacbor[]      = "#e5e5e5";  /* selected border, Vintage ui.text.focus */



static const char col_black[]        = "#000000"; // background
static const char col_red[]          = "#ff5774";
static const char col_green[]        = "#6ae98a";
static const char col_yellow[]       = "#ffe099";
static const char col_blue[]         = "#ff7a99";
static const char col_magenta[]      = "#e0b2a5";
static const char col_cyan[]         = "#efdaa1";
static const char col_white[]        = "#bfbfbf"; // foreground

static const char col_bblack[]       = "#4d4d4d";
static const char col_bred[]         = "#ff6580";
static const char col_bgreen[]       = "#70f893";
static const char col_byellow[]      = "#ffe6ad";
static const char col_bblue[]        = "#ff8ba6";
static const char col_bmagenta[]     = "#e8c4bb";
static const char col_bcyan[]        = "#ffe8ac";
static const char col_bwhite[]       = "#e6e6e6";

static const char col_cursor[]       = "#ff7a99"; // cursor (regular4)
static const char col_currev[]       = "#efdaa1"; // reverse-cursor (regular6)
static const char col_bg[]           = "#000000"; // background
static const char col_fg[]           = "#bfbfbf"; // text


static const char *colors[][3] = {
    /*               fg           bg          border  */
    [SchemeNorm] = { col_fg,      col_bg,     col_bblack },
    [SchemeSel]  = { col_cursor,  col_black,  col_blue   },
};

/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };



#define RULE(...) { .monitor = -1, __VA_ARGS__ },


static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class      instance    title       tags mask     isfloating   monitor */
	// { "Gimp",     NULL,       NULL,       0,            1,           -1 },
	// { "Firefox",  NULL,       NULL,       1 << 8,       0,           -1 },
	//

	RULE(.class    = "TelegramDesktop", .tags = 1 << 4)

	RULE(.class = "Gimp", .tags = 1 << 4)
  RULE(.class = "Firefox", .tags = 1 << 1)
};

/* layout(s) */
static const float mfact     = 0.52; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 0;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */


#define FORCE_VSPLIT 1  /* nrowgrid layout: force two clients to always split vertically */

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },    /* first entry is default */
	{ "><>",      NULL },    /* no layout function means floating behavior */
	{ "[M]",      monocle },

	{ "[@]",      spiral },
	{ "[\\]",     dwindle },
	{ "H[]",      deck },
	{ "TTT",      bstack },
	{ "===",      bstackhoriz },
	{ "HHH",      grid },
	{ "###",      nrowgrid },
	{ "---",      horizgrid },
	{ ":::",      gaplessgrid },
	{ "|M|",      centeredmaster },
	{ ">M>",      centeredfloatingmaster },
	// { "><>",      NULL },    /* no layout function means floating behavior */
	{ NULL,       NULL },
};



/* key definitions */
#define MODKEY Mod1Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */


static const char *dmenucmd[] = {
    "dmenu_run",
      "-m", dmenumon,
      "-fn", dmenufont,
      "-nb", col_bg,        /* #000000 */
      "-nf", col_fg,        /* #bfbfbf */
      "-sb", col_cursor,    /* #ff7a99 */
      "-sf", col_fg,        /* #bfbfbf */
    NULL
};

// static const char *termcmd[]  = { "alacritty", NULL };
static const char *termcmd[] = { "st", NULL }; 

static const char *flameshot[] = { "flameshot", "gui", NULL };

static const Key keys[] = {
	/* modifier                     key        function        argument */
	{ MODKEY,                       XK_p,      spawn,          {.v = dmenucmd } },
	{ MODKEY|ShiftMask,             XK_Return, spawn,          {.v = termcmd } },
	{ MODKEY|ShiftMask,             XK_f,      togglefullscr,  {0}             },
	//mod1+shift+f = fullscreen
	{ MODKEY,                       XK_b,      togglebar,      {0} },
	{ MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
	{ MODKEY,                       XK_i,      incnmaster,     {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_d,      incnmaster,     {.i = -1 } },
	{ MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
	{ MODKEY,                       XK_l,      setmfact,       {.f = +0.05} },
	{ MODKEY,                       XK_Return, zoom,           {0} },
	{ MODKEY,                       XK_Tab,    view,           {0} },
	{ MODKEY|ShiftMask,             XK_c,      killclient,     {0} },
	{ MODKEY,                       XK_t,      setlayout,      {.v = &layouts[0]} },
	{ MODKEY,                       XK_f,      setlayout,      {.v = &layouts[1]} },
	{ MODKEY,                       XK_m,      setlayout,      {.v = &layouts[2]} },
	{ MODKEY,                       XK_s,      setlayout,      {.v = &layouts[3]} },  /* spiral */
  { MODKEY,                       XK_d,      setlayout,      {.v = &layouts[4]} },  /* dwindle */
  { MODKEY,                       XK_e,      setlayout,      {.v = &layouts[5]} },  /* deck       (dEck)      */
  { MODKEY,                       XK_r,      setlayout,     {.v = &layouts[6]} },  /* bstack     (sTack)     */
  { MODKEY,                       XK_v,      setlayout,     {.v = &layouts[7]} },  /* bstackhoriz(vertiCal) */
  { MODKEY,                       XK_g,      setlayout,     {.v = &layouts[8]} },  /* grid      (Grid)      */
  { MODKEY,                       XK_n,      setlayout,     {.v = &layouts[9]} },  /* nrowgrid (Number of rows) */
  { MODKEY,                       XK_x,      setlayout,     {.v = &layouts[10]}},  /* horizgrid(eXplode horiz.) */
  { MODKEY,                       XK_y,      setlayout,     {.v = &layouts[11]}},  /* gaplessgrid (emptY?)    */
  { MODKEY,                       XK_c,      setlayout,     {.v = &layouts[12]}},  /* centeredmaster (Center) */
  { MODKEY,                       XK_z,      setlayout,     {.v = &layouts[13]}},  /* centeredfloatingmaster (cZ = floZy?) */
	{ MODKEY,                       XK_space,  setlayout,      {0} },
	{ MODKEY|ShiftMask,             XK_space,  togglefloating, {0} },
	{ MODKEY,                       XK_0,      view,           {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_0,      tag,            {.ui = ~0 } },
	{ MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
	{ MODKEY,                       XK_period, focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_comma,  tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_period, tagmon,         {.i = +1 } },

	//custom
  { 0,                            XK_Print,                   spawn,          {.v = flameshot} },
  { 0,                            XK_ISO_Next_Group,          spawn,          SHCMD("pkill -RTMIN+10 dwmblocks") },
  { MODKEY|ShiftMask,             XK_j,      movestack,      {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_k,      movestack,      {.i = -1 } },

	{ MODKEY|ShiftMask,             XK_h,      setcfact,       {.f = +0.25} },
	{ MODKEY|ShiftMask,             XK_l,      setcfact,       {.f = -0.25} },
	{ MODKEY|ShiftMask,             XK_o,      setcfact,       {.f =  0.00} },
	{ MODKEY|Mod4Mask,              XK_u,      incrgaps,       {.i = +1 } },
	{ MODKEY|Mod4Mask|ShiftMask,    XK_u,      incrgaps,       {.i = -1 } },
	{ MODKEY|Mod4Mask,              XK_i,      incrigaps,      {.i = +1 } },
	{ MODKEY|Mod4Mask|ShiftMask,    XK_i,      incrigaps,      {.i = -1 } },
	{ MODKEY|Mod4Mask,              XK_o,      incrogaps,      {.i = +1 } },
	{ MODKEY|Mod4Mask|ShiftMask,    XK_o,      incrogaps,      {.i = -1 } },
	{ MODKEY|Mod4Mask,              XK_6,      incrihgaps,     {.i = +1 } },
	{ MODKEY|Mod4Mask|ShiftMask,    XK_6,      incrihgaps,     {.i = -1 } },
	{ MODKEY|Mod4Mask,              XK_7,      incrivgaps,     {.i = +1 } },
	{ MODKEY|Mod4Mask|ShiftMask,    XK_7,      incrivgaps,     {.i = -1 } },
	{ MODKEY|Mod4Mask,              XK_8,      incrohgaps,     {.i = +1 } },
	{ MODKEY|Mod4Mask|ShiftMask,    XK_8,      incrohgaps,     {.i = -1 } },
	{ MODKEY|Mod4Mask,              XK_9,      incrovgaps,     {.i = +1 } },
	{ MODKEY|Mod4Mask|ShiftMask,    XK_9,      incrovgaps,     {.i = -1 } },
	{ MODKEY|Mod4Mask,              XK_0,      togglegaps,     {0} },
	{ MODKEY|Mod4Mask|ShiftMask,    XK_0,      defaultgaps,    {0} },

	TAGKEYS(                        XK_1,                      0)
	TAGKEYS(                        XK_2,                      1)
	TAGKEYS(                        XK_3,                      2)
	TAGKEYS(                        XK_4,                      3)
	TAGKEYS(                        XK_5,                      4)
	TAGKEYS(                        XK_6,                      5)
	TAGKEYS(                        XK_7,                      6)
	TAGKEYS(                        XK_8,                      7)
	TAGKEYS(                        XK_9,                      8)
	{ MODKEY|ShiftMask,             XK_q,      quit,           {0} },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
	{ ClkWinTitle,          0,              Button2,        zoom,           {0} },
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

