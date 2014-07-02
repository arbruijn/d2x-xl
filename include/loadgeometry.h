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

#ifndef _LOADGEOMETRY_H
#define _LOADGEOMETRY_H

#define MINE_VERSION        20  // Current version expected
#define COMPATIBLE_VERSION  16  // Oldest version that can safely be loaded.

struct mtfi {
	uint16_t  fileinfo_signature;
	uint16_t  fileinfoVersion;
	int32_t     fileinfo_sizeof;
};    // Should be same as first two fields below...

struct mfi {
	uint16_t  fileinfo_signature;
	uint16_t  fileinfoVersion;
	int32_t     fileinfo_sizeof;
	int32_t     header_offset;      // Stuff common to game & editor
	int32_t     header_size;
	int32_t     editor_offset;      // Editor specific stuff
	int32_t     editor_size;
	int32_t     segment_offset;
	int32_t     segment_howmany;
	int32_t     segment_sizeof;
	int32_t     newseg_verts_offset;
	int32_t     newseg_verts_howmany;
	int32_t     newseg_verts_sizeof;
	int32_t     group_offset;
	int32_t     group_howmany;
	int32_t     group_sizeof;
	int32_t     vertex_offset;
	int32_t     vertex_howmany;
	int32_t     vertex_sizeof;
	int32_t     texture_offset;
	int32_t     texture_howmany;
	int32_t     texture_sizeof;
	int32_t     walls_offset;
	int32_t     walls_howmany;
	int32_t     walls_sizeof;
	int32_t     triggers_offset;
	int32_t     triggers_howmany;
	int32_t     triggers_sizeof;
	int32_t     links_offset;
	int32_t     links_howmany;
	int32_t     links_sizeof;
	int32_t     object_offset;      // Object info
	int32_t     object_howmany;
	int32_t     object_sizeof;
	int32_t     unused_offset;      // was: doors_offset
	int32_t     unused_howmamy;     // was: doors_howmany
	int32_t     unused_sizeof;      // was: doors_sizeof
	int16_t   level_shake_frequency, level_shake_duration;
	// Shakes every level_shake_frequency seconds
	// for level_shake_duration seconds (on average, Random).  In 16ths second.
	int32_t     secret_return_segment;
	CFixMatrix secret_return_orient;

	int32_t     dl_indices_offset;
	int32_t     dl_indices_howmany;
	int32_t     dl_indices_sizeof;

	int32_t     delta_light_offset;
	int32_t     delta_light_howmany;
	int32_t     delta_light_sizeof;

	int32_t     segment2_offset;
	int32_t     segment2_howmany;
	int32_t     segment2_sizeof;

};

struct mh {
	int32_t num_vertices;
	int32_t num_segments;
};

struct me {
	int32_t current_seg;
	int32_t newsegment_offset;
	int32_t newsegment_size;
	int32_t Curside;
	int32_t Markedsegp;
	int32_t Markedside;
	int32_t Groupsegp[10];
	int32_t Groupside [10];
	int32_t num_groups;
	int32_t current_group;
	//int32_t numObjects;
};

extern struct mtfi mine_top_fileinfo;   // Should be same as first two fields below...
extern struct mfi mine_fileinfo;
extern struct mh mine_header;
extern struct me mine_editor;

// returns 1 if error, else 0
int32_t game_load_mine(char * filename);

void ReadColor (CFile& cf, CFaceColor *pc, int32_t bFloatData, int32_t bRegisterColor);

// loads from an already-open file
// returns 0=everything ok, 1=old version, -1=error
int32_t load_mine_data(CFile& cf);
int32_t LoadMineSegmentsCompiled (CFile& cf);
void CreateFaceList (void);
void ComputeNearestLights (int32_t nLevel);
void InitTexColors (void);

extern int16_t tmap_xlate_table [];
extern fix Level_shake_frequency, Level_shake_duration;
extern int32_t Secret_return_segment;
extern CFixMatrix Secret_return_orient;

/* stuff for loading descent.pig of descent 1 */
extern int16_t ConvertD1Texture(int16_t d1_tmap_num, int32_t bForce);
extern int32_t d1_tmap_num_unique(int16_t d1_tmap_num); //is d1_tmap_num's texture only in d1?

int32_t LoadMineGaugeSize (void);
int32_t SortLightsGaugeSize (void);
int32_t SegDistGaugeSize (void);

#endif /* _LOADGEOMETRY_H */
