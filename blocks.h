// Modify this file to change what commands output to your statusbar, and recompile using the make command.
static const Block blocks[] = {
	/* Icon */	/* Command */				/* Update Interval */	/* Update Signal */
	// { "",		"music",					0,					20 },
	//{ "",		"ethernet",					60,					0 },
	//{ "",		"get_volume_pamixer",		0,					10 },
	//{ "",		"disk_perc",				300,				0 },
	//{ "",		"cpu_temp",					60,					0 },
        //
    { "",       "kb_lang",                      1,                 4},
    { "",       "bat_level",                    1,                 5},
    { "",       "get_vol",                      1,                 10},
    { "",	"calendar",			60000000,          15},
    { "",	"local_time",			60,	           0},
};

// Sets delimeter between status commands. NULL character ('\0') means no delimeter.
static char delim[] = " | ";
static unsigned int delimLen = 3;
