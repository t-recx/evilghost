#include <allegro.h>
#include <stdio.h>
#include <aldumb.h>

#define LOG printf

#define DIRECTION_LEFT 0
#define DIRECTION_RIGHT 1
#define DIRECTION_UP 2
#define DIRECTION_DOWN 3

#define MAP_MAX_WIDTH 20
#define MAP_MAX_HEIGHT 14
#define MAP_TERRAIN 0
#define MAP_OBJECTS 1
#define MAP_BORDERS 2

#define MAX_TILES 256
#define MAX_TILE_BMPS 8

#define MAX_PATH_POINTS 64
#define MAX_PATHS 64

#define MAX_PERSONS 64
#define MAX_PERSON_TYPES 16

#define MAX_TEXT_BALOONS 16
#define TIME_TEXT_BALOON 500
#define TEXT_BALOON_MSG_NONE 0
#define TEXT_BALOON_MSG_LURE 1
#define TEXT_BALOON_MSG_SCARE 2
#define TEXT_BALOON_MSG_CONFUSED 3

#define PLAYER_SEEN_BY_EMPLOYEE 1

#define PERSON_TIME_TO_DIE 2000//2000

#define PERSON_TYPE_NORMAL 0
#define PERSON_TYPE_CAT 1
#define PERSON_TYPE_EMPLOYEE 2

#define PERSON_STATE_DEAD 0
#define PERSON_STATE_OKAY 1
#define PERSON_STATE_SCARED 2
#define PERSON_STATE_LURED 3
#define MAX_PERSON_STATES PERSON_STATE_LURED

#define BMP_PLAYER 0
#define BMP_COLLISION 1

#define PLAYER_STATE_NOT_MOVING 0
#define PLAYER_STATE_MOVING 1

#define END_LEVEL_TIME_IS_UP 1
#define END_LEVEL_CLEARED 2
#define END_LEVEL_ESCAPE 3
#define END_LEVEL_PLAYER_SEEN 4

volatile int sc=0;
volatile int msecs=0;
void sc_add(){sc++;}END_OF_FUNCTION (sc_add);
void msecs_add(){msecs++;}END_OF_FUNCTION (msecs_add);
COLOR_MAP global_trans_table;

typedef struct
{
 BITMAP *background_bmp;
 BITMAP *characters_bmp;
 BITMAP *mansion_bmp;
 BITMAP *grass_bmp;
 DUH *menu_theme;
 int text_timer;
 int letters_timer;
 int show_text;
 int lx, ly;
} GAME_MENU;

typedef struct
{
 int x, y;
 int timer;
 int time_to_end;
 int active;
 int message;
 char text[256];
} TEXT_BALOON;

typedef struct
{
 int x, y;
} PATH_POINT;

typedef struct
{
 int npath_points_used;
 PATH_POINT path_point[MAX_PATH_POINTS];
} PATH;

typedef struct
{
 int x, y;
 int direction;
 int type;
 int state;
 int path_used;
 int current_path_point;
 int fc, fctimer;
 int mvtimer;
 int time_to_next_frame;
 int time_to_death;
} PERSON;

typedef struct
{
 int time_to_next_frame;
 char symbol;
 BITMAP *bmp[MAX_TILE_BMPS];
} TILE;

typedef struct
{
 int tile_number;
 int fc;
 int fctimer;
} MAP_BLOCK;

typedef struct
{
 int seconds;
 int msecs;
 int npaths;
 int npersons, ncats, nemployees;
 PATH path[MAX_PATHS];
 PERSON person[MAX_PERSONS];
 MAP_BLOCK map[3][MAP_MAX_HEIGHT][MAP_MAX_WIDTH];
} LEVEL;

typedef struct
{
 int x, y;
 int lives;
 int state;
 int direction;
 int fc, fctimer;
 int baloontimer[2];
 /* TYPE, STATE, DIRECTION, FC */
 BITMAP *bmp[2][2][4][8];
} PLAYER;

typedef struct
{
 int bps;
 int points;
 int evil_meter_counter;
 BITMAP *little_border_bmp;
 BITMAP *game_border_bmp;
 BITMAP *dialog_bmp[4];
 BITMAP *collision_bmp;
 BITMAP *player_collision_bmp;
 /* TYPE, STATE, DIRECTION, FC */
 BITMAP *person_bmp[MAX_PERSON_TYPES][MAX_PERSON_STATES][4][8];
 TEXT_BALOON text_baloon[MAX_TEXT_BALOONS];
 TILE tile[MAX_TILES];
 LEVEL level;
 PLAYER player;
 DUH *ingame_theme;
 AL_DUH_PLAYER *dp;
} GAME;

BITMAP *shlogo_bmp;

void rand_update ()
{
 srand (msecs);
}

int rand_ex (int min, int max)
{
 if (max<min) { return (0); }
 if (max==min) { return (max); }
 return (rand ()%(max+1 - min)) + min;
}

void take_screenshot ()
{
 FILE *fd= NULL;
 BITMAP *save= NULL;
 char filename[200];
 int x=0;
 char a[210];
 PALETTE pal;

 get_palette (pal);

 save= create_sub_bitmap (screen, 0, 0, SCREEN_W, SCREEN_H);

 for (x=0;;x++)
 {
  sprintf (filename, "screenshot%d.bmp", x);
  fd= fopen (filename, "r");
  if (fd==NULL) break;
 }
 
 sprintf (a, "as %s", filename);
 save_bitmap (filename, save, pal);
 alert ("Picture saved", a, "", "&Ok", NULL, 'o', 0);
 destroy_bitmap (save);
 if (fd!=NULL)
 {
  fclose (fd);
 }
}

void update_screen (BITMAP *bmp)
{
 acquire_screen ();
 vsync ();
 blit (bmp, screen, 0, 0, 0, 0, bmp->w, bmp->h);
 //stretch_blit(bmp, screen, 0, 0, 640, 480, 0, 0, SCREEN_W, SCREEN_H);
 clear (bmp);
 release_screen ();
}

void shadow_textout_ex (BITMAP *bmp, FONT *font, char *s, int x, int y, int fg, int bg)
{
 textout_ex(bmp, font, s, x, y+1, makecol (0, 0, 0), bg);
 textout_ex(bmp, font, s, x+1, y, makecol (0, 0, 0), bg);
 textout_ex(bmp, font, s, x+1, y+1, makecol (0, 0, 0), bg);
 textout_ex(bmp, font, s, x, y, fg, bg);
}

void shadow_textout_right_ex (BITMAP *bmp, FONT *font, char *s, int x, int y, int fg, int bg)
{
 textout_right_ex(bmp, font, s, x, y+1, makecol (0, 0, 0), bg);
 textout_right_ex(bmp, font, s, x+1, y, makecol (0, 0, 0), bg);
 textout_right_ex(bmp, font, s, x+1, y+1, makecol (0, 0, 0), bg);
 textout_right_ex(bmp, font, s, x, y, fg, bg);
}

void shadow_textout_centre_ex (BITMAP *bmp, FONT *font, char *s, int x, int y, int fg, int bg)
{
 textout_centre_ex(bmp, font, s, x, y+1, makecol (0, 0, 0), bg);
 textout_centre_ex(bmp, font, s, x+1, y, makecol (0, 0, 0), bg);
 textout_centre_ex(bmp, font, s, x+1, y+1, makecol (0, 0, 0), bg);
 textout_centre_ex(bmp, font, s, x, y, fg, bg);
}

int load_tileset (GAME *game, char *filename)
{
 int x= 0, y;
 char file_location[320];
 FILE *fp= fopen (filename, "r");

																LOG ("Loading tileset...\n");
 if (fp==NULL) return (-1);
																LOG ("Opened %s successfully!\n", filename);

																LOG ("Lets check how many tiles we've got until now...\n");
 for (y=0;y<MAX_TILES;y++)
 {
  if (game->tile[y].bmp[0]==NULL)
  {
   x= y; 
   break;
  }
 }
 y=0;
																LOG ("Hmm, apparently we have: %d.\n", x);

 while (!feof (fp))
 {
																LOG ("Loading tile #%d\n", x);
  fscanf (fp, "%c\n", &game->tile[x].symbol);
																LOG ("-> Symbol #%c\n", game->tile[x].symbol);
  fscanf (fp, "%d\n", &game->tile[x].time_to_next_frame);
																LOG ("-> TTNF #%d\n", game->tile[x].time_to_next_frame);
  y=0;
  strcpy (file_location, "");
  while (strcmp (file_location, "NULL"))
  {
																LOG ("-> Loading Bitmap #%d\n", y);
   fscanf (fp, "%s", file_location);
   if (strcmp(file_location, "NULL")==0)
   {
																LOG ("--> End of bitmaps\n");
    game->tile[x].bmp[y]= NULL;
   }
   else
   {
																LOG ("--> Location #%s\n", file_location);
    game->tile[x].bmp[y]= load_bitmap (file_location, NULL);
    if (game->tile[x].bmp[y]==NULL)								LOG ("--> ERROR: Bitmap not loaded!\n");
   }
   y++;
  }
  fscanf(fp, "\n");
  x++;
 }
 
																LOG ("Loading of tileset file: Done!");
 fclose (fp);
 return (0);
}

void destroy_tileset (GAME *game)
{
 int x, y;
 
 for (x=0;x<MAX_TILES;x++)
 {
  for (y=0;y<MAX_TILE_BMPS;y++)
  {
   if (game->tile[x].bmp[y]!=NULL) destroy_bitmap (game->tile[x].bmp[y]);
  }
 }
}

int symbol_to_number (GAME *game, char symbol)
{
 int x;

 for (x=0;x<MAX_TILES;x++) { if (game->tile[x].symbol==symbol) return (x); }
 return (0);
}

int load_path (GAME *game, char *filename)
{
 int x= 0, y;
 char file_location[320];
 FILE *fp= fopen (filename, "r");

																LOG ("Loading pathfile...\n");
 if (fp==NULL) return (-1);
																LOG ("Opened %s successfully!\n", filename);

																LOG ("Lets check how many paths we've got until now...\n");
 for (y=0;y<MAX_PATHS;y++)
 {
  if (game->level.path[y].npath_points_used==0)
  {
   x= y; 
   break;
  }
 }
 y=0;
																LOG ("Hmm, apparently we have: %d.\n", x);
 while (!feof (fp))
 {
																LOG ("Loading path point #%d: ", y);
  fscanf (fp, "%d %d\n", &game->level.path[x].path_point[y].x, &game->level.path[x].path_point[y].y);
																LOG ("(%d, %d)\n", game->level.path[x].path_point[y].x, game->level.path[x].path_point[y].y);
  y++;
 }
 game->level.path[x].npath_points_used= y;
 
																LOG ("Loading of path file: Done!");
 fclose (fp);
 return (0);
}

void create_person (GAME *game, int type)
{
 int x;
 for (x=0;x<MAX_PERSONS;x++)
 {
  if (game->level.person[x].state==PERSON_STATE_DEAD)
  {
																LOG ("Person %d: type: %d\n", x, type);
   game->level.person[x].path_used= rand_ex (0, game->level.npaths-1);
   game->level.person[x].current_path_point= rand_ex (0, game->level.path[game->level.person[x].path_used].npath_points_used-1);
																LOG ("path %d: ", game->level.person[x].path_used);
   game->level.person[x].x= game->level.path[game->level.person[x].path_used].path_point[game->level.person[x].current_path_point].x;
   game->level.person[x].y= game->level.path[game->level.person[x].path_used].path_point[game->level.person[x].current_path_point].y;
																LOG ("(%d, %d)\n", game->level.person[x].x, game->level.person[x].y);
   game->level.person[x].state= PERSON_STATE_OKAY;
   game->level.person[x].time_to_next_frame= rand_ex (150, 450);
   game->level.person[x].time_to_death= 0;
   game->level.person[x].type= type;
   break;
  }
 }
}

void start_persons (GAME *game)
{
 int x;
 for (x=0;x<game->level.npersons;x++)
 {
  create_person (game, PERSON_TYPE_NORMAL);
 }
 for (x=0;x<game->level.ncats;x++)
 {
  create_person (game, PERSON_TYPE_CAT);
 }
 for (x=0;x<game->level.nemployees;x++)
 {
  create_person (game, PERSON_TYPE_EMPLOYEE);
 }
}

void load_level (GAME *game, char *filename)
{
 int x, y, z;
 char st[320];
 char symbol, tileset_file[320];
 FILE *fp= fopen (filename, "r");

 game->level.npaths=0;
 game->level.npersons= 0;
 game->level.ncats= 0;
 game->level.nemployees= 0;

																LOG ("Loading level %s...\n", filename);
 if (fp==NULL)													LOG ("ERROR: Unable to load level!\n");
 fscanf (fp, "%d %d\n", &game->player.x, &game->player.y);
 fscanf (fp, "%d\n", &game->level.seconds);
 fscanf (fp, "%d %d %d\n", &game->level.npersons, &game->level.ncats, &game->level.nemployees);
																LOG ("Persons: %d Cats: %d Employees: %d\n", game->level.npersons, game->level.ncats, game->level.nemployees);

 while (strcmp (st, "tilesetend"))
 {
  fscanf (fp, "%s\n", st);
  if (strcmp (st, "tilesetend"))
  {
   load_tileset (game, st);
  }
 }

 while (strcmp (st, "pathsend"))
 {
  fscanf (fp, "%s\n", st);
  if (strcmp (st, "pathsend"))
  {
   game->level.npaths++;
   load_path (game, st);
  }
 }

 for (z=0;z<3;z++)
 {
  for (y=0;y<MAP_MAX_HEIGHT;y++)
  {
   for (x=0;x<MAP_MAX_WIDTH;x++)
   {
    fscanf (fp, "%c", &symbol);
    game->level.map[z][y][x].tile_number= symbol_to_number (game, symbol);
   }
   fscanf (fp, "\n");
  } 
  fscanf (fp, "\n\n");
 }

 fclose (fp);
																LOG ("OK!\n");
}

void init_tileset (GAME *game)
{
 int x, y;
 
 for (x=0;x<MAX_TILES;x++)
 {
  for (y=0;y<MAX_TILE_BMPS;y++)
  {
   game->tile[x].bmp[y]= NULL;
  }
  game->tile[x].symbol= 0;
  game->tile[x].time_to_next_frame= 0;
 }
}

void init_game_map (GAME *game)
{
 int x, y, z;
 for (z=0;z<3;z++)
 {
  for (y=0;y<MAP_MAX_HEIGHT;y++)
  {
   for (x=0;x<MAP_MAX_WIDTH;x++)
   {
    game->level.map[z][y][x].tile_number= 0;
    game->level.map[z][y][x].fc= 0;
    game->level.map[z][y][x].fctimer= msecs;
   }
  }
 }
}

void init_paths (GAME *game)
{
 int x;
 for (x=0;x<MAX_PATHS;x++)
 {
  game->level.path[x].npath_points_used= 0;
 }
}

void init_persons (GAME *game)
{
 int x;

 for (x=0;x<MAX_PERSONS;x++)
 {
  game->level.person[x].x= 0;
  game->level.person[x].y= 0;
  game->level.person[x].type= PERSON_TYPE_NORMAL;
  game->level.person[x].direction= 0;
  game->level.person[x].state= PERSON_STATE_DEAD;
  game->level.person[x].path_used= 0;
  game->level.person[x].current_path_point= 0;
  game->level.person[x].fc= 0;
  game->level.person[x].fctimer= 0;
  game->level.person[x].mvtimer= 0;
  game->level.person[x].time_to_next_frame= 300;
  game->level.person[x].time_to_death= 0;
 }
}

void init_text_baloons (GAME *game)
{
 int x;
 for (x=0;x<MAX_TEXT_BALOONS;x++)
 {
  game->text_baloon[x].timer= 0;
  game->text_baloon[x].time_to_end= 0;
  game->text_baloon[x].message= TEXT_BALOON_MSG_NONE;
  game->text_baloon[x].active= 0;
  game->text_baloon[x].x= 0;
  game->text_baloon[x].y= 0;
  strcpy (game->text_baloon[x].text, ""); 
 }
}

void init_player (GAME *game)
{
 game->player.x= 0;
 game->player.y= 0;
 game->player.direction= DIRECTION_RIGHT;
 game->player.fc= 0;
 game->player.fctimer= 0;
 game->player.baloontimer[0]= 0;
 game->player.baloontimer[1]= 0;
}

void init_level (GAME *game, char *filename)
{
 init_tileset (game);
 init_game_map (game);
 init_paths (game);
 init_persons (game);
 init_text_baloons (game);
 init_player (game);
 load_level (game, filename);
 start_persons (game);
 game->evil_meter_counter= 1;
 game->level.msecs= msecs;
}

void destroy_level (GAME *game)
{
 destroy_tileset (game);
}

void draw_player (GAME *game, BITMAP *bmp)
{
 if (game->player.state==PLAYER_STATE_NOT_MOVING)
 {
  draw_trans_sprite (bmp, game->player.bmp[BMP_PLAYER][game->player.state][game->player.direction][game->player.fc], game->player.x, game->player.y);
 }
 else
 {
  draw_sprite (bmp, game->player.bmp[BMP_PLAYER][game->player.state][game->player.direction][game->player.fc], game->player.x, game->player.y);
 }
}

void draw_collision_player (GAME *game, BITMAP *bmp)
{
 if (game->player.state==PLAYER_STATE_NOT_MOVING)
 {
  //draw_trans_sprite (bmp, game->player.bmp[BMP_PLAYER][game->player.state][game->player.direction][game->player.fc], game->player.x, game->player.y);
 }
 else
 {
  draw_sprite (bmp, game->player_collision_bmp, game->player.x, game->player.y);
 }
}

void draw_terrain (GAME *game, BITMAP *bmp)
{
 int x, y;
 for (y=0;y<MAP_MAX_HEIGHT;y++)
 {
  for (x=0;x<MAP_MAX_WIDTH;x++)
  {
   draw_sprite (bmp, game->tile[game->level.map[MAP_TERRAIN][y][x].tile_number].bmp[game->level.map[MAP_TERRAIN][y][x].fc],
                x*16, y*16);
   draw_sprite (bmp, game->tile[game->level.map[MAP_BORDERS][y][x].tile_number].bmp[game->level.map[MAP_BORDERS][y][x].fc],
                x*16, y*16);
  }
 }
}


void draw_collision_bmp (GAME *game, BITMAP *bmp)
{
 int x, y;
 for (y=0;y<MAP_MAX_HEIGHT;y++)
 {
  for (x=0;x<MAP_MAX_WIDTH;x++)
  {
   draw_sprite (bmp, game->tile[game->level.map[MAP_BORDERS][y][x].tile_number].bmp[game->level.map[MAP_BORDERS][y][x].fc],
                x*16, y*16);
  }
  if (game->player.y>=y*16 && game->player.y<=y*16+16)
  {
   draw_collision_player (game, bmp);
  }
 }
}

void draw_map_objects (GAME *game, BITMAP *bmp)
{
 int x, y, z;

 for (y=0;y<MAP_MAX_HEIGHT;y++)
 {
  for (x=0;x<MAP_MAX_WIDTH;x++)
  {
   draw_sprite (bmp, 
                game->tile[game->level.map[MAP_OBJECTS][y][x].tile_number].bmp[game->level.map[MAP_OBJECTS][y][x].fc], 
                x*16, 
                y*16+16-game->tile[game->level.map[MAP_OBJECTS][y][x].tile_number].bmp[game->level.map[MAP_OBJECTS][y][x].fc]->h);
  }
  for (z=0;z<MAX_PERSONS;z++)
  {
   if (game->level.person[z].state!=PERSON_STATE_DEAD && (game->level.person[z].y>=y*16+9 && game->level.person[z].y<=y*16+16+9))
   {
    if (game->level.person[z].state==PERSON_STATE_SCARED && game->level.person[z].time_to_death<=msecs-PERSON_TIME_TO_DIE/2)
    {
     draw_trans_sprite (bmp, game->person_bmp[game->level.person[z].type][game->level.person[z].state][game->level.person[z].direction][game->level.person[z].fc],
                 game->level.person[z].x, game->level.person[z].y);
    }
    else
    {
     draw_sprite (bmp, game->person_bmp[game->level.person[z].type][game->level.person[z].state][game->level.person[z].direction][game->level.person[z].fc],
                 game->level.person[z].x, game->level.person[z].y);
    }
   }
  }
  if (game->player.y>=y*16 && game->player.y<=y*16+16)
  {
   draw_player (game, bmp);
  }
 }
}

void create_text_baloon (GAME *game, char *text, int xx, int y, int message, int time_to_end)
{
 int x;
 for (x=0;x<MAX_TEXT_BALOONS;x++)
 {
  if (!game->text_baloon[x].active)
  {
   game->text_baloon[x].time_to_end= time_to_end;
   game->text_baloon[x].x= xx;
   game->text_baloon[x].y= y;
   game->text_baloon[x].active= 1;
   game->text_baloon[x].timer= msecs;
   game->text_baloon[x].message= message;
   strcpy (game->text_baloon[x].text, text);
   break;
  }
 }
}

void draw_text_baloons (GAME *game, BITMAP *bmp)
{
 int x, l, y;
 for (x=0;x<MAX_TEXT_BALOONS;x++)
 {
  if (game->text_baloon[x].active)
  {
   l= text_length (font, game->text_baloon[x].text)-text_length(font, "12");
   draw_sprite (bmp, game->dialog_bmp[1], game->text_baloon[x].x, game->text_baloon[x].y);
   for (y=0;y<l;y++)
   {
    draw_sprite (bmp, game->dialog_bmp[2], game->text_baloon[x].x+16+y, game->text_baloon[x].y);
   }
   draw_sprite (bmp, game->dialog_bmp[3], game->text_baloon[x].x+16+l, game->text_baloon[x].y);
   textout_ex (bmp, font, game->text_baloon[x].text, game->text_baloon[x].x+8, game->text_baloon[x].y+8/2, makecol (0,0,0)/*216*/, -1);
   draw_sprite (bmp, game->dialog_bmp[0], game->text_baloon[x].x+4, game->text_baloon[x].y+14);
  }
 }
}

int get_people_left_to_scare (GAME *game)
{
 int x, i=0;
 for (x=0;x<MAX_PERSONS;x++)
 {
  if (game->level.person[x].type==PERSON_TYPE_NORMAL && game->level.person[x].state!=PERSON_STATE_DEAD)
  {
   i++;
  }
 }
 return (i);
}

void draw_game_border (GAME *game, BITMAP *bmp)
{
 char st[256];
 draw_sprite (bmp, game->game_border_bmp, 0, 240-16);
 sprintf (st, "x%d", game->player.lives);
 shadow_textout_ex(bmp, font, st, 19, (240-16)+4, makecol(255, 255, 255), -1);
 sprintf (st, "%d:%02d", (game->level.seconds-(msecs/1000-game->level.msecs/1000)) / 60,
    					 (game->level.seconds-(msecs/1000-game->level.msecs/1000)) % 60);
 shadow_textout_ex(bmp, font, st, 50, (240-16)+4, makecol(255, 255, 255), -1);
 sprintf (st, "%d", game->points);
 shadow_textout_right_ex (bmp, font, st, 317, (240-16)+4, makecol (255, 255, 255), -1);
 sprintf (st, "%d", get_people_left_to_scare (game));
 shadow_textout_ex (bmp, font, st, 95, (240-16)+4, makecol (255, 255, 255), -1);
}

void draw_level (GAME *game, BITMAP *bmp)
{
 draw_terrain (game, bmp);
 draw_map_objects (game, bmp);
 draw_collision_bmp (game, game->collision_bmp);
 draw_text_baloons (game, bmp);
 draw_game_border (game, bmp);
 //draw_collision_bmp (game, bmp);
 //draw_player (game, bmp);
}

void text_baloons_logic (GAME *game)
{
 int x;
 for (x=0;x<MAX_TEXT_BALOONS;x++)
 {
  if (game->text_baloon[x].active)
  {
   if (game->text_baloon[x].timer<=msecs-game->text_baloon[x].time_to_end) 
   {
    game->text_baloon[x].active= 0;
   }
  }
 }
}

void player_logic (GAME *game)
{
 int x, time_to_next_frame= 100;
 int mv[2][4]= { { -1, 1, 0, 0 }, { 0, 0, -1, 1 } };

 game->player.state= PLAYER_STATE_NOT_MOVING;
 for (x=0;x<4;x++)
 {
  if (key[KEY_Z] && game->player.baloontimer[0]<=msecs-500)
  {
   create_text_baloon (game, "Uhm?", game->player.x+8, game->player.y-20, TEXT_BALOON_MSG_LURE, 500);
   game->player.baloontimer[0]= msecs;
  }
  if (key[KEY_X] && game->player.baloontimer[1]<=msecs-500)
  {
   create_text_baloon (game, "BWAH!", game->player.x+8, game->player.y-20, TEXT_BALOON_MSG_SCARE, 500);
   game->player.baloontimer[1]= msecs;
  }
  if (key[KEY_LEFT+x]) 
  {
   game->player.direction = x;
   if (key[KEY_LEFT] && (key[KEY_UP] || key[KEY_DOWN])) game->player.direction = DIRECTION_LEFT;
   if (key[KEY_RIGHT] && (key[KEY_UP] || key[KEY_DOWN])) game->player.direction = DIRECTION_RIGHT;
   game->player.x += mv[0][x];
   game->player.y += mv[1][x];
   game->player.state= PLAYER_STATE_MOVING;
  }
 }
 if (game->player.state==PLAYER_STATE_MOVING)
 {
  if (game->player.fctimer<=msecs-time_to_next_frame)
  {
   if (game->player.bmp[BMP_PLAYER][game->player.state][game->player.direction][game->player.fc+1]!=NULL)
   {
    game->player.fc++;
   }
   else
   {
    game->player.fc= 0;
   }
   game->player.fctimer= msecs;
  }
 }
 else
 {
  game->player.fc= 0;
  game->player.fctimer= msecs;
 }
}

int distance_to_player (GAME *game, int p)
{
 int x, y;

 if (game->player.x>game->level.person[p].x)
 {
  x= game->player.x-game->level.person[p].x;
 }
 else
 {
  x= game->level.person[p].x-game->player.x;
 }
 if (game->player.y>game->level.person[p].y)
 {
  y= game->player.y-game->level.person[p].y;
 }
 else
 {
  y= game->level.person[p].y-game->player.y;
 }
 return (1+abs(x+y));
}

int give_score (GAME *game, int p)
{
 int x, y, z;
 if (game->level.person[p].type!=PERSON_TYPE_NORMAL) return (0);
 if (game->player.x>game->level.person[p].x)
 {
  x= game->player.x-game->level.person[p].x;
 }
 else
 {
  x= game->level.person[p].x-game->player.x;
 }
 if (game->player.y>game->level.person[p].y)
 {
  y= game->player.y-game->level.person[p].y;
 }
 else
 {
  y= game->level.person[p].y-game->player.y;
 }
 game->points+= 1000/(1+abs(x+y));
 LOG ("%d %d %d\n", x, y, x+y);
 for (z=0;z<MAX_TEXT_BALOONS;z++)
 {
  if (game->text_baloon[z].active)
  {
   if (game->text_baloon[z].x>=game->level.person[p].x-64 && game->text_baloon[z].x<=game->level.person[p].x+32 &&
       game->text_baloon[z].y>=game->level.person[p].y-64 && game->text_baloon[z].y<=game->level.person[p].y+32)
   {
    if (game->text_baloon[z].message==TEXT_BALOON_MSG_SCARE && game->level.person[p].state==PERSON_STATE_SCARED)
    {
     game->points+= 50;
    }
   }
  }
 }
 return (0);
}

int change_person_state (GAME *game, int p, int new_state)
{
 char *confused_messages[4]= { "Hello?", "Who is there?", "Hum?", "?" };
 char *panic_messages[4]= { "AHH!!", "NOES!", "It's a GHOST!", "!" };
 if (game->level.person[p].state!=new_state)
 {
  if (game->level.person[p].state==PERSON_STATE_SCARED) return (0);
  game->level.person[p].state= new_state;
  if (new_state==PERSON_STATE_SCARED)
  {
   give_score (game, p);
   create_text_baloon (game, panic_messages[rand_ex(0, 3)], game->level.person[p].x+3, game->level.person[p].y-20, TEXT_BALOON_MSG_CONFUSED, 1000);
   game->level.person[p].time_to_death= msecs;
  }
  if (new_state==PERSON_STATE_LURED)
  {
   create_text_baloon (game, confused_messages[rand_ex(0, 3)], game->level.person[p].x+3, game->level.person[p].y-20, TEXT_BALOON_MSG_CONFUSED, 1000);
  }
  game->level.person[p].fc= 0;
 }
 return (0);
}

void people_observe_environment (GAME *game, int p)
{
 int x, y, z, object_seen;

 // check for ghosts:
 switch (game->level.person[p].direction)
 {
  case DIRECTION_LEFT:
   for (x=game->level.person[p].x;x>0;x--)
   {
    object_seen= getpixel (game->collision_bmp, x, game->level.person[p].y+7);
    if (object_seen==15 || object_seen==255) // player
    {
     change_person_state (game, p, PERSON_STATE_SCARED);
																LOG ("PLAYER SEEN! (scary)\n");
    }
    else if (object_seen!=0) // border
    {
     break;
    }  
   }
  break;
  case DIRECTION_RIGHT:
   for (x=game->level.person[p].x;x<320;x++)
   {
    object_seen= getpixel (game->collision_bmp, x, game->level.person[p].y+7);
    if (object_seen==15 || object_seen==255) // player
    {
     change_person_state (game, p, PERSON_STATE_SCARED);
																LOG ("PLAYER SEEN! (scary)\n");
    }
    else if (object_seen!=0) // border
    {
     break;
    }   
   }
  break;
  case DIRECTION_UP:
   for (y=game->level.person[p].y+7;y>0;y--)
   {
    object_seen= getpixel (game->collision_bmp, game->level.person[p].x+3, y);
    if (object_seen==255) // player
    {
     change_person_state (game, p, PERSON_STATE_SCARED);
																LOG ("PLAYER SEEN! (scary)\n");
    }
    else if (object_seen!=0) // border
    {
     break;
    }  
   }
  break;
  case DIRECTION_DOWN:
   for (y=game->level.person[p].y+7;y<240;y++)
   {
    object_seen= getpixel (game->collision_bmp, game->level.person[p].x+3, y);
    if (object_seen==255) // player
    {
     change_person_state (game, p, PERSON_STATE_SCARED);
																LOG ("PLAYER SEEN! (scary)\n");
    }
    else if (object_seen!=0) // border
    {
     break;
    }  
   }
  break;
 }

 // check if someone is calling in a 96x96 radius:
 for (z=0;z<MAX_TEXT_BALOONS;z++)
 {
  if (game->text_baloon[z].active)
  {
   if (game->text_baloon[z].x>=game->level.person[p].x-64 && game->text_baloon[z].x<=game->level.person[p].x+32 &&
       game->text_baloon[z].y>=game->level.person[p].y-64 && game->text_baloon[z].y<=game->level.person[p].y+32)
   {
    if (game->text_baloon[z].message==TEXT_BALOON_MSG_LURE) change_person_state (game, p, PERSON_STATE_LURED);
   }
  }
 }
}

void choose_resolution ()
{
 int end_game= 0;
 BITMAP *swap= create_bitmap (320, 240);
 BITMAP *swap2= create_bitmap (768, 480);
 BITMAP *cat= load_bitmap ("media/bmps/people/catw1.bmp", NULL);
 SAMPLE *blip= load_sample ("media/blip.wav");
 SAMPLE *blip2= load_sample ("media/blip2.wav");
 int selection= 1;
 int selected_w=640, selected_h= 480;
 int ps;
 clear (swap);
 clear (swap2);

 while (!end_game)
 {
  while (sc>0 && !end_game)
  {
   ps= selection;
   if (key[KEY_ENTER])
   {
    end_game= 1;
    play_sample (blip2, 255, 128, 1000, 0);
   }
   if (key[KEY_UP])
   {
    selection= 1;
   }
   if (key[KEY_DOWN])
   {
    selection= 2;
   }
   if (ps!=selection)
   {
    play_sample (blip, 255, 128, 1000, 0);
   }
   sc--;
  }

  clear_to_color(swap, 215);
  shadow_textout_centre_ex (swap, font, "SELECT VIDEO FORMAT", 320/2, 240/2-20, makecol(255, 255, 255), -1);
  shadow_textout_centre_ex (swap, font, "FULLSCREEN", 320/2, 240/2, makecol(255, 255, 255), -1);
  shadow_textout_centre_ex (swap, font, "WIDESCREEN", 320/2, 240/2+12, makecol(255, 255, 255), -1);
  if (selection==1) draw_sprite (swap, cat, 320/2-text_length (font, "FULLSCREEN")/2-8, 240/2);
  if (selection==2) draw_sprite (swap, cat, 320/2-text_length (font, "FULLSCREEN")/2-8, 240/2+12);
  stretch_blit(swap, swap2, 0, 0, 320, 240, 0, 0, 640, 480);
  update_screen (swap2);
 }

 if (selection==1)
 {
  selected_w=640; selected_h= 480;
 }
 else
 {
  selected_w=768; selected_h= 480;
 }
 if (selected_w!=SCREEN_W)
 {
  if (set_gfx_mode (GFX_AUTODETECT, selected_w, selected_h, 0, 0))
  {
   set_gfx_mode (GFX_AUTODETECT, selected_w, selected_h, 640, 480);
   alert ("Unable to set up", "video format:", allegro_error, "OK", NULL, 0, 0);
  }
 }

 destroy_bitmap (swap);
 destroy_bitmap (swap2);
 fade_out (1);
}


void people_check_walls (GAME *game, int p, int px, int py)
{
 if (getpixel (game->collision_bmp, game->level.person[p].x, game->level.person[p].y)==240 ||
     getpixel (game->collision_bmp, game->level.person[p].x, game->level.person[p].y+3)==240 ||
     getpixel (game->collision_bmp, game->level.person[p].x, game->level.person[p].y+7)==240)
 {
  game->level.person[p].x= px;
 }     
 if (getpixel (game->collision_bmp, game->level.person[p].x+5, game->level.person[p].y)==240 ||
     getpixel (game->collision_bmp, game->level.person[p].x+5, game->level.person[p].y+3)==240 ||
     getpixel (game->collision_bmp, game->level.person[p].x+5, game->level.person[p].y+7)==240)
 {
  game->level.person[p].x= px;
 }     
/*
 if (getpixel (game->collision_bmp, game->level.person[p].x, game->level.person[p].y)==240 ||
     getpixel (game->collision_bmp, game->level.person[p].x+3, game->level.person[p].y)==240 ||
     getpixel (game->collision_bmp, game->level.person[p].x+6, game->level.person[p].y)==240)
 {
  game->level.person[p].y= py;
 }     */
 if (getpixel (game->collision_bmp, game->level.person[p].x, game->level.person[p].y+7)==240 ||
     getpixel (game->collision_bmp, game->level.person[p].x+3, game->level.person[p].y+7)==240 ||
     getpixel (game->collision_bmp, game->level.person[p].x+5, game->level.person[p].y+7)==240)
 {
  game->level.person[p].y= py;
 }     
}

void cat_logic (GAME *game, int x)
{
 int px, py;
 px= game->level.person[x].x;
 py= game->level.person[x].y;
 if (game->level.person[x].state==PERSON_STATE_OKAY)
 {
  if (game->level.person[x].mvtimer<=msecs-game->level.person[x].time_to_next_frame)
  {
   if (game->level.person[x].x<game->level.path[game->level.person[x].path_used].path_point[game->level.person[x].current_path_point].x)
   {
     game->level.person[x].x++;
     game->level.person[x].direction= DIRECTION_RIGHT;
   }
   if (game->level.person[x].x>game->level.path[game->level.person[x].path_used].path_point[game->level.person[x].current_path_point].x)
   {
    game->level.person[x].x--;
    game->level.person[x].direction= DIRECTION_LEFT;
   }
   if (game->level.person[x].y<game->level.path[game->level.person[x].path_used].path_point[game->level.person[x].current_path_point].y)
   {
    game->level.person[x].y++;
    game->level.person[x].direction= DIRECTION_DOWN;
   }
   if (game->level.person[x].y>game->level.path[game->level.person[x].path_used].path_point[game->level.person[x].current_path_point].y) 
   {
    game->level.person[x].y--;
    game->level.person[x].direction= DIRECTION_UP;
   }
   if (px==game->level.person[x].x && py==game->level.person[x].y)
   {
    game->level.person[x].current_path_point++;
    if (game->level.person[x].current_path_point>=game->level.path[game->level.person[x].path_used].npath_points_used)
    {
     game->level.person[x].x= game->level.path[game->level.person[x].path_used].path_point[0].x;
     game->level.person[x].y= game->level.path[game->level.person[x].path_used].path_point[0].y;
     game->level.person[x].current_path_point= 0;
    }
   }
   game->level.person[x].mvtimer= msecs;
   if (!rand_ex (0, 100))
   {
    game->level.person[x].state= PERSON_STATE_LURED;
    game->level.person[x].time_to_next_frame= game->level.person[x].time_to_next_frame*10;
    game->level.person[x].fc=2;
   }
  }
 }
 else if (game->level.person[x].state==PERSON_STATE_LURED)
 {
  // In cats lured=sleeping
  if (game->level.person[x].mvtimer<=msecs-game->level.person[x].time_to_next_frame)
  {
   if (game->level.person[x].fc==0)
   {
    game->level.person[x].state= PERSON_STATE_OKAY;
    game->level.person[x].time_to_next_frame= game->level.person[x].time_to_next_frame/10;
    if (!rand_ex (0, 10))
    {  
     create_text_baloon (game, "meow", game->level.person[x].x, game->level.person[x].y-24, TEXT_BALOON_MSG_NONE, 1000);
    }
   }
   game->level.person[x].mvtimer= msecs;
  }
 }

 if (game->level.person[x].fctimer<=msecs-game->level.person[x].time_to_next_frame) 
 {
  if (game->person_bmp[game->level.person[x].type][game->level.person[x].state][game->level.person[x].direction][game->level.person[x].fc+1]!=NULL)
  {
   game->level.person[x].fc++;
  }
  else
  {
   game->level.person[x].fc= 0;
  }
  game->level.person[x].fctimer= msecs;
 }

 people_check_walls (game, x, px, py);
}

int employee_logic (GAME *game, int x)
{
 int px, py;
 px= game->level.person[x].x;
 py= game->level.person[x].y;
 if (game->level.person[x].state==PERSON_STATE_OKAY)
 {
  if (game->level.person[x].mvtimer<=msecs-game->level.person[x].time_to_next_frame)
  {
   if (game->level.person[x].x<game->level.path[game->level.person[x].path_used].path_point[game->level.person[x].current_path_point].x)
   {
     game->level.person[x].x++;
     game->level.person[x].direction= DIRECTION_RIGHT;
   }
   if (game->level.person[x].x>game->level.path[game->level.person[x].path_used].path_point[game->level.person[x].current_path_point].x)
   {
    game->level.person[x].x--;
    game->level.person[x].direction= DIRECTION_LEFT;
   }
   if (game->level.person[x].y<game->level.path[game->level.person[x].path_used].path_point[game->level.person[x].current_path_point].y)
   {
    game->level.person[x].y++;
    game->level.person[x].direction= DIRECTION_DOWN;
   }
   if (game->level.person[x].y>game->level.path[game->level.person[x].path_used].path_point[game->level.person[x].current_path_point].y) 
   {
    game->level.person[x].y--;
    game->level.person[x].direction= DIRECTION_UP;
   }
   if (px==game->level.person[x].x && py==game->level.person[x].y)
   {
    game->level.person[x].current_path_point++;
    if (game->level.person[x].current_path_point>=game->level.path[game->level.person[x].path_used].npath_points_used)
    {
     game->level.person[x].x= game->level.path[game->level.person[x].path_used].path_point[0].x;
     game->level.person[x].y= game->level.path[game->level.person[x].path_used].path_point[0].y;
     game->level.person[x].current_path_point= 0;
    }
   }
   game->level.person[x].mvtimer= msecs;
  }
 }
 else if (game->level.person[x].state==PERSON_STATE_SCARED)
 {
  // if employees get scared it means that they saw the game player, the level must end and one game life must be taken from the player
  create_text_baloon (game, "Mr. Applegate!", game->level.person[x].x+3, game->level.person[x].y-20, TEXT_BALOON_MSG_NONE, 1000);
  return (PLAYER_SEEN_BY_EMPLOYEE);
 }
 else if (game->level.person[x].state==PERSON_STATE_LURED)
 {
  if (game->level.person[x].mvtimer<=msecs-game->level.person[x].time_to_next_frame)
  {
   if (game->level.person[x].x<game->player.x+8)
   {
    game->level.person[x].x++;
    game->level.person[x].direction= DIRECTION_RIGHT;
   }
   if (game->level.person[x].x>game->player.x+8)
   {
    game->level.person[x].x--;
    game->level.person[x].direction= DIRECTION_LEFT;
   }
   if (game->level.person[x].y<game->player.y+8)
   {
    game->level.person[x].y++;
    game->level.person[x].direction= DIRECTION_DOWN;
   }
   if (game->level.person[x].y>game->player.y+8) 
   {
    game->level.person[x].y--;
    game->level.person[x].direction= DIRECTION_UP;
   }
   game->level.person[x].mvtimer= msecs;
  }
 }
 if (game->level.person[x].fctimer<=msecs-game->level.person[x].time_to_next_frame) 
 {
  if (game->person_bmp[game->level.person[x].type][game->level.person[x].state][game->level.person[x].direction][game->level.person[x].fc+1]!=NULL)
  {
   game->level.person[x].fc++;
  }
  else
  {
   game->level.person[x].fc= 0;
  }
  game->level.person[x].fctimer= msecs;
 }
 people_check_walls (game, x, px, py);
 people_observe_environment (game, x);
 return (0);
}

int people_logic (GAME *game)
{
 int x, px, py;

 for (x=0;x<MAX_PERSONS;x++)
 {
  if (game->level.person[x].state==PERSON_STATE_DEAD) continue;
  if (game->level.person[x].type==PERSON_TYPE_CAT)
  {
   cat_logic (game, x);
   continue;
  }
  if (game->level.person[x].type==PERSON_TYPE_EMPLOYEE)
  {
   if (employee_logic (game, x))
   {
    return (PLAYER_SEEN_BY_EMPLOYEE);
   }
   continue;
  }
  px= game->level.person[x].x;
  py= game->level.person[x].y;
  if (game->level.person[x].state==PERSON_STATE_OKAY)
  {
   if (game->level.person[x].mvtimer<=msecs-game->level.person[x].time_to_next_frame)
   {
    if (game->level.person[x].x<game->level.path[game->level.person[x].path_used].path_point[game->level.person[x].current_path_point].x)
    {
      game->level.person[x].x++;
      game->level.person[x].direction= DIRECTION_RIGHT;
    }
    if (game->level.person[x].x>game->level.path[game->level.person[x].path_used].path_point[game->level.person[x].current_path_point].x)
    {
     game->level.person[x].x--;
     game->level.person[x].direction= DIRECTION_LEFT;
    }
    if (game->level.person[x].y<game->level.path[game->level.person[x].path_used].path_point[game->level.person[x].current_path_point].y)
    {
     game->level.person[x].y++;
     game->level.person[x].direction= DIRECTION_DOWN;
    }
    if (game->level.person[x].y>game->level.path[game->level.person[x].path_used].path_point[game->level.person[x].current_path_point].y) 
    {
     game->level.person[x].y--;
     game->level.person[x].direction= DIRECTION_UP;
    }
    if (px==game->level.person[x].x && py==game->level.person[x].y)
    {
     game->level.person[x].current_path_point++;
     if (game->level.person[x].current_path_point>=game->level.path[game->level.person[x].path_used].npath_points_used)
     {
      game->level.person[x].x= game->level.path[game->level.person[x].path_used].path_point[0].x;
      game->level.person[x].y= game->level.path[game->level.person[x].path_used].path_point[0].y;
      game->level.person[x].current_path_point= 0;
     }
    }
    game->level.person[x].mvtimer= msecs;
   }
  }
  else if (game->level.person[x].state==PERSON_STATE_SCARED)
  {
   //game->level.person[x].time_to_next_frame= 50+abs((game->level.person[x].x-game->player.x)+(game->level.person[x].y-game->player.y));
   game->level.person[x].time_to_next_frame= distance_to_player (game, x)/2;
   //LOG ("%d\n", 100-distance_to_player (game, x)*10);
   if (game->level.person[x].mvtimer<=msecs-game->level.person[x].time_to_next_frame)
   {
    if (game->level.person[x].x<game->player.x+8)
    {
      game->level.person[x].x--;
      game->level.person[x].direction= DIRECTION_LEFT;
    }
    if (game->level.person[x].x>game->player.x+8)
    {
     game->level.person[x].x++;
     game->level.person[x].direction= DIRECTION_RIGHT;
    }
    if (game->level.person[x].y<game->player.y+8)
    {
     game->level.person[x].y--;
     game->level.person[x].direction= DIRECTION_UP;
    }
    if (game->level.person[x].y>game->player.y+8) 
    {
     game->level.person[x].y++;
     game->level.person[x].direction= DIRECTION_DOWN;
    }
    game->level.person[x].mvtimer= msecs;
   }
   if (game->level.person[x].time_to_death<=msecs-PERSON_TIME_TO_DIE) game->level.person[x].state= PERSON_STATE_DEAD; // Heart attack, maybe?
  }
  else if (game->level.person[x].state==PERSON_STATE_LURED)
  {
   if (game->level.person[x].mvtimer<=msecs-game->level.person[x].time_to_next_frame)
   {
    if (game->level.person[x].x<game->player.x+8)
    {
      game->level.person[x].x++;
      game->level.person[x].direction= DIRECTION_RIGHT;
    }
    if (game->level.person[x].x>game->player.x+8)
    {
     game->level.person[x].x--;
     game->level.person[x].direction= DIRECTION_LEFT;
    }
    if (game->level.person[x].y<game->player.y+8)
    {
     game->level.person[x].y++;
     game->level.person[x].direction= DIRECTION_DOWN;
    }
    if (game->level.person[x].y>game->player.y+8) 
    {
     game->level.person[x].y--;
     game->level.person[x].direction= DIRECTION_UP;
    }
    game->level.person[x].mvtimer= msecs;
   }
  }

  if (game->level.person[x].fctimer<=msecs-game->level.person[x].time_to_next_frame) 
  {
   if (game->person_bmp[game->level.person[x].type][game->level.person[x].state][game->level.person[x].direction][game->level.person[x].fc+1]!=NULL)
   {
    game->level.person[x].fc++;
   }
   else
   {
    game->level.person[x].fc= 0;
   }
   game->level.person[x].fctimer= msecs;
  }

  people_check_walls (game, x, px, py);
  people_observe_environment (game, x);
 }
 return (0);
}

int level_logic (GAME *game)
{
 if (key[KEY_ESC]) return (END_LEVEL_ESCAPE);
 if (key[KEY_F8]) choose_resolution ();
 if (key[KEY_F10]) take_screenshot ();
 player_logic (game);
 if (people_logic (game)) return (END_LEVEL_PLAYER_SEEN);
 text_baloons_logic (game);
 if (!get_people_left_to_scare (game)) return (END_LEVEL_CLEARED);
 if (game->level.seconds-(msecs/1000-game->level.msecs/1000)<=0)
 {
  return (END_LEVEL_TIME_IS_UP);
 }
 return (0);
}

void draw_evil_meter (GAME *game, BITMAP *bmp)
{
 int ec= game->evil_meter_counter;
 stretch_sprite (bmp, game->little_border_bmp, 640, 480-32, 128, 32);
 shadow_textout_centre_ex (bmp, font, "EVIL", 640+64, 480-36+8+4, makecol(255, 255, 255), -1);
 shadow_textout_centre_ex (bmp, font, "METER", 640+64, 480-36+8+4+10, makecol(255, 255, 255), -1);
 rectfill (bmp, 640+48, 32+380, 640+128-48, 32+380-game->points/50/game->evil_meter_counter, 18);
 rect (bmp, 640+48, 32, 640+128-48, 32+380, 17);
 if (game->points/50>380)
 {
  game->evil_meter_counter++;
 }
 if (ec!=game->evil_meter_counter)
 {
  game->player.lives++;
 }
}

void draw_sh_logo (BITMAP *bmp)
{
 shadow_textout_centre_ex (bmp, font, "EVIL", 640+64, 480-36+8+4, makecol(255, 255, 255), -1);
 shadow_textout_centre_ex (bmp, font, "METER", 640+64, 480-36+8+4+10, makecol(255, 255, 255), -1);
 rectfill (bmp, 640, 0, 640+128, 480, 216);
 draw_sprite (bmp, shlogo_bmp, 640+128/2-shlogo_bmp->w/2, 480/2-shlogo_bmp->h/2);
}

void level_time_is_up (GAME *game)
{
 int end_game= 0;
 BITMAP *swap= create_bitmap (320, 240);
 BITMAP *swap2= create_bitmap (768, 480);

 clear (swap);
 clear (swap2);

 while (!end_game)
 {
  while (sc>0 && !end_game)
  {
   if (key[KEY_ENTER])
   {
    end_game= 1;
   }
   al_poll_duh (game->dp);
   sc--;
  }

  clear_to_color(swap, 215);
  shadow_textout_centre_ex (swap, font, "TIME'S UP!", 320/2, 240/2, makecol(255, 255, 255), -1);
  stretch_blit(swap, swap2, 0, 0, 320, 240, 0, 0, 640, 480);
  draw_evil_meter (game, swap2);
  update_screen (swap2);
 }

 destroy_bitmap (swap);
 destroy_bitmap (swap2);
 game->player.lives--;
}

void level_player_seen (GAME *game)
{
/*
 int end_game= 0;
 BITMAP *swap= create_bitmap (320, 240);

 clear (swap);

 while (!end_game)
 {
  while (sc>0 && !end_game)
  {
   if (key[KEY_ENTER])
   {
    end_game= 1;
   }
   sc--;
  }

  clear_to_color(swap, 215);
  shadow_textout_centre_ex (swap, font, "TIME'S UP!", 320/2, 240/2, makecol(255, 255, 255), -1);
  update_screen (swap);
 }

 destroy_bitmap (swap);
*/
 game->player.lives--;
}

void level_cleared (GAME *game)
{
 int end_game= 0;
 BITMAP *swap= create_bitmap (320, 240);
 BITMAP *swap2= create_bitmap (768, 480);

 clear (swap);
 clear (swap2);

 while (!end_game)
 {
  while (sc>0 && !end_game)
  {
   al_poll_duh (game->dp);
   if (key[KEY_ENTER])
   {
    end_game= 1;
   }
   sc--;
  }

  clear_to_color(swap, 215);
  shadow_textout_centre_ex (swap, font, "LEVEL'S CLEARED!", 320/2, 240/2, makecol(255, 255, 255), -1);
  stretch_blit(swap, swap2, 0, 0, 320, 240, 0, 0, 640, 480);
  draw_evil_meter (game, swap2);
  update_screen (swap2);
 }

 destroy_bitmap (swap);
 destroy_bitmap (swap2);
}

int play_level (GAME *game, char *filename)
{
 int end_level= 0;
 BITMAP *swap= create_bitmap (320, 240);
 BITMAP *swap2= create_bitmap (768, 480);
 BITMAP *collision_bmp = create_bitmap (320, 240);
 
 clear (swap);
 clear (swap2);
 clear (collision_bmp);
																LOG ("Playing level: %s\n", filename);
 init_level (game, filename);

 install_int_ex (sc_add, BPS_TO_TIMER (game->bps));

 while (!end_level)
 {
  while (sc>0 && !end_level)
  {
   end_level= level_logic (game);
   al_poll_duh (game->dp);
   sc--;
  }
  clear (game->collision_bmp);
  draw_level (game, swap);
  stretch_blit(swap, swap2, 0, 0, 320, 240, 0, 0, 640, 480);
  draw_evil_meter (game, swap2);
  update_screen (swap2);
  clear (swap);
 }

 switch (end_level)
 {
  case END_LEVEL_TIME_IS_UP: // time's up
   level_time_is_up (game);
  break;
  case END_LEVEL_CLEARED: // level cleared
   level_cleared (game);
  break;
  case END_LEVEL_PLAYER_SEEN:
   level_player_seen (game);
   draw_level (game, swap);
   shadow_textout_centre_ex (swap, font, "YOU GOT SPOTTED!", 320/2+80/2, 240-16+4, makecol(255, 255, 255), -1);
   stretch_blit(swap, swap2, 0, 0, 320, 240, 0, 0, 640, 480);
   draw_evil_meter (game, swap2);
   update_screen (swap2);
   while (!key[KEY_ENTER])
   {
    al_poll_duh (game->dp);
    if (key[KEY_F10]) take_screenshot ();
   }
  break;
  case END_LEVEL_ESCAPE: // ESCAPE
  break;
 }

 destroy_bitmap (swap);
 destroy_bitmap (swap2);

 destroy_level (game);
 return (end_level);
}

void init_person_bmps (GAME *game)
{
 int x, y, z, i;
 for (i=0;i<MAX_PERSON_TYPES;i++)
 {
  for (x=0;x<MAX_PERSON_STATES;x++)
  {
   for (y=0;y<4;y++)
   {
    for (z=0;z<8;z++)
    {
     game->person_bmp[i][x][y][z]= NULL;
    }
   }
  }
 }

 /* REGULAR PEOPLE */
 for (x=0;x<4;x++)
 {
  game->person_bmp[0][PERSON_STATE_OKAY][x][0]= load_bitmap ("media/bmps/people/personw1.bmp", NULL);
  game->person_bmp[0][PERSON_STATE_OKAY][x][1]= load_bitmap ("media/bmps/people/person.bmp", NULL);
  game->person_bmp[0][PERSON_STATE_OKAY][x][2]= load_bitmap ("media/bmps/people/personw2.bmp", NULL);
  game->person_bmp[0][PERSON_STATE_OKAY][x][3]= load_bitmap ("media/bmps/people/person.bmp", NULL);


  game->person_bmp[0][PERSON_STATE_SCARED][x][0]= load_bitmap ("media/bmps/people/personws1.bmp", NULL);
  game->person_bmp[0][PERSON_STATE_SCARED][x][1]= load_bitmap ("media/bmps/people/personws.bmp", NULL);
  game->person_bmp[0][PERSON_STATE_SCARED][x][2]= load_bitmap ("media/bmps/people/personws2.bmp", NULL);
  game->person_bmp[0][PERSON_STATE_SCARED][x][3]= load_bitmap ("media/bmps/people/personws.bmp", NULL);

  game->person_bmp[0][PERSON_STATE_LURED][x][0]= load_bitmap ("media/bmps/people/personw1.bmp", NULL);
  game->person_bmp[0][PERSON_STATE_LURED][x][1]= load_bitmap ("media/bmps/people/person.bmp", NULL);
  game->person_bmp[0][PERSON_STATE_LURED][x][2]= load_bitmap ("media/bmps/people/personw2.bmp", NULL);
  game->person_bmp[0][PERSON_STATE_LURED][x][3]= load_bitmap ("media/bmps/people/person.bmp", NULL);
 }

 /* CATS */
 for (x=0;x<4;x++)
 {
  game->person_bmp[PERSON_TYPE_CAT][PERSON_STATE_OKAY][x][0]= load_bitmap ("media/bmps/people/catw2.bmp", NULL);
  game->person_bmp[PERSON_TYPE_CAT][PERSON_STATE_OKAY][x][1]= load_bitmap ("media/bmps/people/catw2.bmp", NULL);
  game->person_bmp[PERSON_TYPE_CAT][PERSON_STATE_OKAY][x][2]= load_bitmap ("media/bmps/people/catw3.bmp", NULL);
  game->person_bmp[PERSON_TYPE_CAT][PERSON_STATE_OKAY][x][3]= load_bitmap ("media/bmps/people/catw3.bmp", NULL);

  game->person_bmp[PERSON_TYPE_CAT][PERSON_STATE_SCARED][x][0]= load_bitmap ("media/bmps/people/catw2.bmp", NULL);
  game->person_bmp[PERSON_TYPE_CAT][PERSON_STATE_SCARED][x][1]= load_bitmap ("media/bmps/people/catw2.bmp", NULL);
  game->person_bmp[PERSON_TYPE_CAT][PERSON_STATE_SCARED][x][2]= load_bitmap ("media/bmps/people/catw3.bmp", NULL);
  game->person_bmp[PERSON_TYPE_CAT][PERSON_STATE_SCARED][x][3]= load_bitmap ("media/bmps/people/catw3.bmp", NULL);

  game->person_bmp[PERSON_TYPE_CAT][PERSON_STATE_LURED][x][0]= load_bitmap ("media/bmps/people/catstill0.bmp", NULL);
  game->person_bmp[PERSON_TYPE_CAT][PERSON_STATE_LURED][x][1]= load_bitmap ("media/bmps/people/catstill1.bmp", NULL);
  game->person_bmp[PERSON_TYPE_CAT][PERSON_STATE_LURED][x][2]= load_bitmap ("media/bmps/people/catstill0.bmp", NULL);
  game->person_bmp[PERSON_TYPE_CAT][PERSON_STATE_LURED][x][3]= load_bitmap ("media/bmps/people/catstill1.bmp", NULL);
 }

 /* EMPLOYEES */
 for (x=0;x<4;x++)
 {
  game->person_bmp[PERSON_TYPE_EMPLOYEE][PERSON_STATE_OKAY][x][0]= load_bitmap ("media/bmps/people/employeew1.bmp", NULL);
  game->person_bmp[PERSON_TYPE_EMPLOYEE][PERSON_STATE_OKAY][x][1]= load_bitmap ("media/bmps/people/employee.bmp", NULL);
  game->person_bmp[PERSON_TYPE_EMPLOYEE][PERSON_STATE_OKAY][x][2]= load_bitmap ("media/bmps/people/employeew2.bmp", NULL);
  game->person_bmp[PERSON_TYPE_EMPLOYEE][PERSON_STATE_OKAY][x][3]= load_bitmap ("media/bmps/people/employee.bmp", NULL);

  game->person_bmp[PERSON_TYPE_EMPLOYEE][PERSON_STATE_SCARED][x][0]= load_bitmap ("media/bmps/people/employeew1.bmp", NULL);
  game->person_bmp[PERSON_TYPE_EMPLOYEE][PERSON_STATE_SCARED][x][1]= load_bitmap ("media/bmps/people/employee.bmp", NULL);
  game->person_bmp[PERSON_TYPE_EMPLOYEE][PERSON_STATE_SCARED][x][2]= load_bitmap ("media/bmps/people/employeew2.bmp", NULL);
  game->person_bmp[PERSON_TYPE_EMPLOYEE][PERSON_STATE_SCARED][x][3]= load_bitmap ("media/bmps/people/employee.bmp", NULL);

  game->person_bmp[PERSON_TYPE_EMPLOYEE][PERSON_STATE_LURED][x][0]= load_bitmap ("media/bmps/people/employeew1.bmp", NULL);
  game->person_bmp[PERSON_TYPE_EMPLOYEE][PERSON_STATE_LURED][x][1]= load_bitmap ("media/bmps/people/employee.bmp", NULL);
  game->person_bmp[PERSON_TYPE_EMPLOYEE][PERSON_STATE_LURED][x][2]= load_bitmap ("media/bmps/people/employeew2.bmp", NULL);
  game->person_bmp[PERSON_TYPE_EMPLOYEE][PERSON_STATE_LURED][x][3]= load_bitmap ("media/bmps/people/employee.bmp", NULL);
 }
}

void init_bmps (GAME *game)
{
 int x, y;
 char file_location[320];
 char *character_files_rest[]=
 {
// Facing left
  "ghostleft.bmp", "ghostleft.bmp", "ghostleft.bmp", "ghostleft.bmp",
// Facing right  
  "ghostright.bmp", "ghostright.bmp", "ghostright.bmp", "ghostright.bmp",
// Facing up
  "ghostup.bmp", "ghostup.bmp", "ghostup.bmp", "ghostup.bmp",
// Facing down
  "ghostdown.bmp", "ghostdown.bmp", "ghostdown.bmp", "ghostdown.bmp",
 };
 char *character_files_movement[]=
 {
// Facing left
  "ghostleft1.bmp", "ghostleft2.bmp", "ghostleft1.bmp", "ghostleft2.bmp",
// Facing right  
  "ghostright1.bmp", "ghostright2.bmp", "ghostright1.bmp", "ghostright2.bmp",
// Facing up
  "ghostupw1.bmp", "ghostupw2.bmp", "ghostupw1.bmp", "ghostupw2.bmp",
// Facing down
  "ghostdownw1.bmp", "ghostdownw2.bmp", "ghostdownw1.bmp", "ghostdownw2.bmp",
 };

 for (y=0;y<4;y++)
 {
  for (x=0;x<4;x++)
  {
   sprintf(file_location, "media/bmps/%s", character_files_movement[y*4+x]);
   game->player.bmp[BMP_PLAYER][PLAYER_STATE_MOVING][y][x]= load_bitmap (file_location, NULL);
   sprintf(file_location, "media/bmps/collision/%s", character_files_movement[y*4+x]);
//   game->player.bmp[BMP_COLLISION][PLAYER_STATE_MOVING][y][x]= load_bitmap (file_location, NULL);
  }
  game->player.bmp[BMP_PLAYER][PLAYER_STATE_MOVING][y][4]= NULL;
//  game->player.bmp[BMP_COLLISION][PLAYER_STATE_MOVING][y][4]= NULL;
 }

 for (y=0;y<4;y++)
 {
  for (x=0;x<4;x++)
  {
   sprintf(file_location, "media/bmps/%s", character_files_rest[y*4+x]);
   game->player.bmp[BMP_PLAYER][PLAYER_STATE_NOT_MOVING][y][x]= load_bitmap (file_location, NULL);
   sprintf(file_location, "media/bmps/collision/%s", character_files_rest[y*4+x]);
  // game->player.bmp[BMP_COLLISION][PLAYER_STATE_NOT_MOVING][y][x]= load_bitmap (file_location, NULL);
  }
  game->player.bmp[BMP_PLAYER][PLAYER_STATE_NOT_MOVING][y][4]= NULL;
  //game->player.bmp[BMP_COLLISION][PLAYER_STATE_NOT_MOVING][y][4]= NULL;
 }

 init_person_bmps (game);

 game->collision_bmp= create_bitmap (320, 240);
 clear (game->collision_bmp);

 game->player_collision_bmp = load_bitmap ("media/bmps/ghostcollision.bmp", NULL);

 game->dialog_bmp[0]= load_bitmap ("media/bmps/dialogarrow.bmp", NULL);
 game->dialog_bmp[1]= load_bitmap ("media/bmps/dialogleft.bmp", NULL);
 game->dialog_bmp[2]= load_bitmap ("media/bmps/dialogmiddle.bmp", NULL);
 game->dialog_bmp[3]= load_bitmap ("media/bmps/dialogright.bmp", NULL);

 game->game_border_bmp= load_bitmap ("media/bmps/gameborder.bmp", NULL);
 game->little_border_bmp= load_bitmap ("media/bmps/littleborder.bmp", NULL);
}

GAME *init_game ()
{
 GAME *game= NULL;

 game= malloc (1 * sizeof (GAME));

 init_bmps (game);

 game->points= 0;
 game->player.lives= 2;
 game->bps= 60;
 
 game->ingame_theme= dumb_load_it ("media/ingame.it");
 return (game);
}

void destroy_game (GAME *game)
{
 free (game);
}

void game_over (GAME *game)
{
 int end_game= 0;
 BITMAP *swap= create_bitmap (320, 240);
 BITMAP *swap2= create_bitmap (768, 480);
 PALETTE pal;
 get_palette (pal);

 clear (swap);
 clear (swap2);

 while (!end_game)
 {
  while (sc>0 && !end_game)
  {
   al_poll_duh (game->dp);
   if (key[KEY_ENTER])
   {
    end_game= 1;
   }
   sc--;
  }

  clear_to_color(swap, 215);
  shadow_textout_centre_ex (swap, font, "GAME OVER", 320/2, 240/2, makecol(255, 255, 255), -1);
  stretch_blit(swap, swap2, 0, 0, 320, 240, 0, 0, 640, 480);
  draw_evil_meter (game, swap2);
  update_screen (swap2);
 }
 fade_out (1);
 clear (screen);
 fade_in (pal, 64);
 destroy_bitmap (swap);
 destroy_bitmap (swap2);
}

int game ()
{
 GAME *game= NULL;
 int l, return_code;
 char *level[4]= { "level1.txt", "level2.txt", "level3.txt", "" };
																LOG ("Initializing game...");
 game= init_game ();
 if (game==NULL) return (-1);
																LOG ("OK!\n");
 install_int_ex (sc_add, BPS_TO_TIMER (60));

 game->dp= al_start_duh (game->ingame_theme, 2, 0, 1.0f, 4096*2, 48000);

 l= 0;
 while (1)
 {
  return_code= play_level (game, level[l]);
  if (return_code==END_LEVEL_CLEARED)
  {
   l++;
   if (level[l]=="")
   {
    game->bps++;
    l= 0;
   }
  }
  if (return_code==END_LEVEL_ESCAPE) break;
  if (game->player.lives<=0) break;
 }
 al_stop_duh (game->dp);
 rest (200);
 game_over (game);

 install_int_ex (sc_add, BPS_TO_TIMER (60));

 destroy_game (game);
 return (0);
}

void init_pallete ()
{
 PALLETE pal;
 BITMAP *p= load_bitmap ("media/pal.bmp", pal);
 set_pallete (pal);
 create_trans_table(&global_trans_table, pal, 208, 208, 208, NULL);
 color_map = &global_trans_table;
 destroy_bitmap (p);
}

int init_allegro ()
{
 int selected_w=640, selected_h= 480;
																LOG ("Initializing Allegro");
 if (!allegro_init ()) if (!install_keyboard ()) if (!install_timer ()) if (!set_gfx_mode (GFX_AUTODETECT, selected_w, selected_h, 0, 0))
 {
																LOG (".");
  install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL);
  atexit(&dumb_exit);
  dumb_register_stdfiles();
																LOG (".");
  LOCK_VARIABLE (sc);
  LOCK_VARIABLE (msecs);
  LOCK_FUNCTION (sc_add);
  LOCK_FUNCTION (msecs_add);
  install_int_ex (msecs_add, MSEC_TO_TIMER (1));
  install_int_ex (sc_add, BPS_TO_TIMER (60));
																LOG (".");
  font = load_font ("media/font.pcx", NULL, NULL);
  init_pallete ();
      															LOG ("OK!\n");
  return (0);
 }
 return (-1);
}

void destroy_allegro ()
{
 allegro_exit ();
}

GAME_MENU *init_game_menu ()
{
 GAME_MENU *game_menu= NULL;

 game_menu= malloc (1 * sizeof (GAME_MENU));

 game_menu->menu_theme= dumb_load_it ("media/menu.it");

 game_menu->show_text= 0;
 game_menu->text_timer= msecs;
 game_menu->letters_timer= 0;

 game_menu->mansion_bmp= load_bitmap ("media/bmps/mansion.bmp", NULL);
 game_menu->grass_bmp= load_bitmap ("media/bmps/grass.bmp", NULL);
 game_menu->characters_bmp= load_bitmap ("media/bmps/characters4.bmp", NULL);
 game_menu->lx= 30;
 game_menu->ly= 240/2-game_menu->characters_bmp->h/2;
 return (game_menu);
}

void destroy_game_menu (GAME_MENU *game_menu)
{
 unload_duh(game_menu->menu_theme);
 free (game_menu);
}

void draw_game_menu (GAME_MENU *game_menu, BITMAP *bmp)
{

 int x, y;
/*
 for (y=0;y<16;y++)
 {
  for (x=0;x<20;x++)
  {
   draw_sprite (bmp, game_menu->grass_bmp, x*16, y*16);
  }
 }*/
 //for (y=-3;y<16;y++)
 //{
 // for (x=-3;x<20;x++)
 // {
   
 // }
 //}
 clear_to_color (bmp, 213);
 rectfill (bmp, game_menu->lx-4, 0, game_menu->lx+game_menu->characters_bmp->w+3, 240, 240),
 draw_sprite (bmp, game_menu->characters_bmp, game_menu->lx, game_menu->ly);
 draw_sprite (bmp, game_menu->mansion_bmp, 320/2-game_menu->mansion_bmp->w/2, 240/2-game_menu->mansion_bmp->h/2);
 shadow_textout_centre_ex (bmp, font, "EVIL GHOST", 320/2-4, 22, makecol(255, 255, 255), -1);
 shadow_textout_centre_ex (bmp, font, "HAUNTS HOUSE|||", 320/2+4, 32, makecol(255, 255, 255), -1);
 if (game_menu->show_text)
 {
  shadow_textout_centre_ex (bmp, font, "PRESS START BUTTON", 320/2+1, 220, makecol(255, 255, 255), -1);
 }
}

int logic_game_menu (GAME_MENU *game_menu)
{
 if (game_menu->text_timer<=msecs-500)
 {
  game_menu->show_text++;
  if (game_menu->show_text>1) game_menu->show_text= 0;
  game_menu->text_timer= msecs;
 }
 if (game_menu->letters_timer<=msecs-500)
 {
  game_menu->letters_timer= msecs;
 // game_menu->ly++;

/*
  if (game_menu->ly>game_menu->characters_bmp->h)
  {
   game_menu->ly= 0;
  }*/
  if (game_menu->ly>240)
  {
   
   game_menu->ly= -game_menu->characters_bmp->h;
  }

 }
 if (key[KEY_ENTER])
 {
  return (1);
 }
 if (key[KEY_ESC])
 {
  return (2);
 }
 return (0);
}

int game_menu ()
{
 int end_game_menu= 0;
 GAME_MENU *game_menu= NULL;
 BITMAP *swap= create_bitmap (320, 240);
 BITMAP *swap2= create_bitmap (768, 480);
 AL_DUH_PLAYER *dp= NULL;
 PALETTE pal;

 get_palette (pal);

 clear (swap);
 clear (swap2);

 game_menu= init_game_menu ();

 dp= al_start_duh (game_menu->menu_theme, 2, 0, 1.0f, 4096*2, 48000);

 while (!end_game_menu)
 {
  while (sc>0 && !end_game_menu)
  {
   al_poll_duh (dp);
   end_game_menu= logic_game_menu (game_menu);
   if (!end_game_menu) al_poll_duh (dp);
   sc--;
  }
  draw_game_menu (game_menu, swap);
  stretch_blit(swap, swap2, 0, 0, 320, 240, 0, 0, 640, 480);
  draw_sh_logo (swap2);
  update_screen (swap2);
  clear (swap);
  if (end_game_menu)
  {
   al_stop_duh (dp);
   fade_out (1);
   clear (screen);
   fade_in (pal, 64);
  }
 }

 //al_stop_duh (dp);
 destroy_bitmap (swap);
 destroy_game_menu (game_menu);

 return (end_game_menu);
}

void microcat_intro ()
{
 int end_game_intro= 0;
 BITMAP *swap= create_bitmap (320, 240);
 BITMAP *logo[3];
 AL_DUH_PLAYER *dp= NULL;
 DUH *intro_it;
 int fc, fctimer;
 int first_time= 1;
 int total_time;
 PALETTE pal;
 BITMAP *swap2= create_bitmap (768, 480);

 clear (swap);
 clear (swap2);

 logo[0]= load_bitmap ("media/bmps/logo1.bmp", pal);
 logo[1]= load_bitmap ("media/bmps/logo2.bmp", NULL);
 logo[2]= NULL;

 fc= 0;
 clear (screen);
 draw_sprite (swap, logo[fc], 320/2-logo[fc]->w/2, 240/2-logo[fc]->h/2);
 stretch_blit(swap, swap2, 0, 0, 320, 240, 0, 0, 640, 480);
 update_screen (swap2);
 fade_in (pal, 1);

 intro_it= dumb_load_it ("media/purr.it");
 
 fctimer= msecs;
 total_time= msecs;

 sc= 0;
 while (!end_game_intro)
 {
  while (sc>0 && !end_game_intro)
  {
   if (first_time)
   {
    first_time= 0;
    dp= al_start_duh (intro_it, 2, 0, 1.0f, 4096*2, 48000);
    fctimer= msecs;
   }
   if (fctimer<=msecs-850)
   {
    if (logo[fc+1]!=NULL)
    {
     fc++;
    }
    else
    {
     fc= 0;
    }
    fctimer= msecs;
   }

   if (key[KEY_ENTER] || key[KEY_ESC] || total_time<=msecs-4200) end_game_intro= 1;  
   if (!end_game_intro) al_poll_duh (dp);

   sc--;
  }
  
  
  draw_sprite (swap, logo[fc], 320/2-logo[fc]->w/2, 240/2-logo[fc]->h/2);
  stretch_blit(swap, swap2, 0, 0, 320, 240, 0, 0, 640, 480);
  draw_sh_logo (swap2);
  update_screen (swap2);
  clear (swap);

  if (end_game_intro)
  {
   al_stop_duh (dp);
   fade_out (1);
   clear (screen);
   fade_in (pal, 64);
  }
  al_poll_duh (dp);
 }
 
 unload_duh(intro_it);
 destroy_bitmap (swap);
}

int main ()
{
 if (init_allegro ())
 {
  allegro_message ("Error: %s\n", allegro_error);
  return (-1);
 }

 shlogo_bmp= load_bitmap ("media/bmps/shlogo.bmp", NULL);

 choose_resolution ();

 microcat_intro ();

 while (game_menu ()!=2)
 {
  game ();
 }

 destroy_allegro ();
 return (0);
}
END_OF_MAIN ();
