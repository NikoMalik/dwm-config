
//Modify this file to change what commands output to your statusbar, and recompile using the make command.
// static const Block blocks[] = {
// 	/*Icon*/	/*Command*/		/*Update Interval*/	/*Update Signal*/
//   {"┇  ", "xset -q|grep LED| awk '{ if (substr ($10,5,1) == 1) print \"[RU]\"; else print \"[EN]\"; }'", 0, 1},

// 	//TODO: MAKE FREE MEMORY IN %
//   {" ", "sensors | awk '/Core 0/ {print $3}'", 10, 0},

  

// 	{" " , "date '+%d.%m'", 60, 0},
// 	{" ", "date '+%H:%M  '", 5, 0},

// };


static const Block blocks[] = {
    /* Icon  */   /* Command */                                                                                   /* Update Interval */   /* Update Signal */
    {"┇  ",      "xset -q | grep LED | awk '{ if (substr($10,5,1) == 1) print \"[RU]\"; else print \"[EN]\"; }'", 0, 1},
    {" ",      "nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader",                              10, 0},
    {" ", "sensors | awk '/Core 0/ {print $3}'", 10, 0},

    {"[MEM] ",  "awk '/MemTotal/ {total=$2} /MemAvailable/ {avail=$2} END {printf(\"%.0f%%\", avail/total * 100)}' /proc/meminfo", 60, 0},
    {" ",      "date '+%d.%m'",                                                                              60, 0},
    {" ",      "date '+%H:%M  '",                                                                            5, 0},
};

//sets delimiter between status commands. NULL character ('\0') means no delimiter.
static char delim[] = "  ┇  ";
static unsigned int delimLen = 7;
