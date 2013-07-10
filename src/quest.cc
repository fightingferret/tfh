#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"

quest_data* get_quest_index( int i )
{
  if( i < 0 || i >= MAX_QUEST ) 
    return NULL;

  return quest_list[i];
}


/*
 *   LOCATING QUESTS
 */


void do_qwhere( char_data* ch, char *argument )
{
  action_data*     action;
  area_data*         area;
  mprog_data*       mprog;
  obj_clss_data*      obj;
  oprog_data*       oprog;
  room_data*         room;
  species_data*   species;
  int                i, j;
  int               index;
  bool              found  = FALSE;

  index = atoi( argument );

  if( get_quest_index( index ) == NULL ) {
    send( ch, "No quest found with that index.\n\r" );
    return;
    }

  for( area = area_list; area != NULL; area = area->next )
    for( room = area->room_first; room != NULL; room = room->next )
      for( j = 1, action = room->action; action != NULL;
        j++, action = action->next )
        if( search_quest( action->binary, index ) ) {
          found = TRUE;
          page( ch, "  Refered to in action #%d in room #%d\n\r",
            j, room->vnum );
          }

  for( i = 0; i < MAX_SPECIES; i++ ) {
    if( ( species = species_list[i] ) == NULL ) 
      continue;
    for( j = 1, mprog = species->mprog; mprog != NULL;
      j++, mprog = mprog->next )
      if( search_quest( mprog->binary, index ) ) {
        found = TRUE;
        page( ch, "  Refered to in mprog #%d on mob #%d\n\r", j, i );
        }
    }

  for( i = 0; i < MAX_OBJ_INDEX; i++ ) {
    if( ( obj = obj_index_list[i] ) == NULL ) 
      continue; 
    for( j = 1, oprog = obj->oprog; oprog != NULL; j++, oprog = oprog->next )
      if( search_quest( oprog->binary, index ) ) {
        found = TRUE;
        page( ch, "  Refered to in oprog #%d on obj #%d\n\r", j, i );
        }
    }

  if( !found )
    send( ch, "No references to that quest were found.\n\r" );

  return;
}


/*
 *   EDITTING OF QUESTS
 */


void do_qedit( char_data* ch, char *argument )
{
  quest_data*    quest;
  wizard_data*     imm = wizard( ch );
  bool           found = FALSE;  
  int                i;

  if( *argument == '\0' ) {
    for( i = 0; i < MAX_QUEST; i++ ) {
      if( ( quest = quest_list[i] ) == NULL )
        continue;
      found = TRUE;
      page( ch, "[%3d] %-50s\n", i, quest->message );
      }
    if( !found ) 
      send( ch, "No quests found.\n\r" );
    return;
    }
 
  if( matches( argument, "delete" ) ) {
    if( *argument == '\0' ) {
      send( ch, "What quest do you want to delete?\n\r" );
      return;
      }
   
    if( ( i = atoi( argument ) ) < 0 || i >= MAX_QUEST
      || ( quest = quest_list[i] ) == NULL ) {
      send( ch, "Quest not found to remove.\n\r" );
      return;
      }

    extract( imm, offset( &imm->quest_edit, imm ), "quest" );
    quest_list[i] = NULL;
    free_string( quest->message, MEM_QUEST );
    send( ch, "Quest removed.\n\r" );
    delete quest;

    return; 
    }

  if( matches( argument, "new" ) ) {
    for( i = 0; quest_list[i] != NULL; i++ )
      if( i == MAX_QUEST ) {
        send( ch, "Quest space is full.\n\r" );
        return;
        }

    quest              = new quest_data;
    quest->message     = empty_string;
    quest->vnum        = i;  
    quest->points      = 0;
    quest_list[i]      = quest;
    imm->quest_edit = quest;

    send( ch, "Quest created and assigned #%d.\n\r", i );
    return;
    }

  if( ( i = atoi( argument ) ) < 0 || i >= MAX_QUEST ) {
    send( ch, "Quest number is out of range.\n\r" );
    return;
    }

  if( quest_list[i] == NULL ) {
    send( ch, "There is no quest by that number.\n\r" );
    return;
    }

  imm->quest_edit = quest_list[i];
  send( ch, "Qedit now operates on specified quest.\n\r" );

  return;
}  


void do_qset( char_data* ch, char *argument )
{
  quest_data*  quest;
  wizard_data* imm = wizard( ch );
 
  if( *argument == '\0' ) {
    do_qstat( ch, "" );
    return;
  }

  if( ( quest = imm->quest_edit ) == NULL ) {
    send( ch, "You aren't editing any quest.\n\r" );
    return;
    }

  class int_field int_list[] = {
    { "points",        0,  25,  &quest->points },
    { "",              0,   0,  NULL },   
    };

  if( process( int_list, ch, "quest", argument ) )
    return;
  
  class string_field string_list[] = {
    { "message",    MEM_QUEST,  &quest->message,  NULL },
    { "",           0,          NULL,             NULL }   
    };

  if( process( string_list, ch, "quest", argument ) )
    return;

  send( ch, "Unknown parameter.\n\r" );
  return;
}


void do_qstat( char_data* ch, char* )
{
  wizard_data*  imm = wizard( ch );
  quest_data*   quest;

  if( ( quest = imm->quest_edit ) == NULL ) {
    send( ch, "You aren't editing any quest.\n\r" );
    return;
    }

  send( ch, "Message: %s\n\r", quest->message );
  send( ch, "Points: %d\n\r", quest->points );

  return;
}


/*
 *   PLAYER COMMANDS
 */


void do_quests( char_data* ch, char* argument )
{
  quest_data*       quest;
  bool              found  = FALSE; 
  int                   i;
  int               flags;
  int               value;
  const char*   title_msg;
  const char*    none_msg; 

  if( not_player( ch ) ) 
    return;

  if( !get_flags( ch, argument, &flags, "d", "Quests" ) )
    return;

  if( is_set( &flags, 0 ) ) {
    title_msg = "Completed Quests";
    none_msg  = "not completed any";
    value     = -1;
    }
  else {
    title_msg = "Assigned Quests";
    none_msg  = "no unfinished, but completed";
    value     = 1;
    } 

  for( i = 0; i < MAX_QUEST; i++ ) 
    if( ( quest = quest_list[i] ) != NULL 
      && ch->pcdata->quest_flags[i] == value ) {
      if( !found ) {
        page_title( ch, title_msg );
        found = TRUE;
        }
      page_centered( ch, quest->message );   
      }

  if( !found ) 
    send( ch, "You have %s quests.\n\r", none_msg );

  return;
}


/*
 *   EDITTING QUESTS ON PLAYERS
 */


void do_qremove( char_data* ch, char* )
{
  int   i;

  for( i = 0; i < MAX_QUEST; i++ ) 
    ch->pcdata->quest_flags[i] = 0;

  ch->pcdata->quest_pts = 0;

  send( ch, "All quest records erased for your character.\n\r" );
  return;   
}

// Syntax: qflag <num> [U|A|D]
// Effect: Sets qflag #num to 0 (unassigned (default)) or 1 (assigned), or
//         -1 (done).
//
// Alternate Syntax: qflag
// Effect: Dump qflags 8 per row.  Display as A for assigned, D for done.
void do_qflag( char_data* ch, char* argument )
{
  wizard_data* imm = wizard( ch );
  if ( imm == NULL ) 
     return;
  player_data* victim = ( imm->player_edit != NULL ? imm->player_edit : 
                          player( ch ) );
  if ( victim == NULL || victim->pcdata == NULL ) 
     return;

  if ( *argument == '\0' ) {
    page_title( ch, "Quest flags of %s", victim->descr->name );
    page_centered( ch, "A = Assigned, D = Done." );
    int j;
    for ( j = 0; j < MAX_QUEST; j++ ) {
      char quest_char = ' ';
      if ( victim->pcdata->quest_flags[j] == 1 )
	 quest_char = 'A';
      else if ( victim->pcdata->quest_flags[j] == -1 )
         quest_char = 'D';
      page( ch, "%5d (%1c)", j, quest_char );
      if( j%8 == 7 )
        page( ch, "\n\r" );
    }
    if( j%8 != 0 )
      page( ch, "\n\r" );
    return;
  }

  // Extract the quest flag number (0-128).
  int i;
  if ( !number_arg( argument, i )) {
    send( ch, "Syntax: qflag <num> [A|D]\n\r" );
    return;
  }
  if ( i < 0 || i >= MAX_QUEST ) {
    send( ch, "Qflag number out of range.\n\r" );
    return;
  }

  // Last (optional) argument indicates how to set the quest flag.
  if ( *argument == '\0' || *argument == 'U' ) {
    victim->pcdata->quest_flags[i] = 0;
    send( ch, "Qflag %d on %s set to Unassigned.\n\r", i,
          victim == ch ? "yourself" : victim->descr->name );
  }
  else if ( *argument == 'A' ) {
    victim->pcdata->quest_flags[i] = 1;
    send( ch, "Qflag %d on %s set to Assigned.\n\r", i,
          victim == ch ? "yourself" : victim->descr->name );
  }
  else if ( *argument == 'D' ) {
    victim->pcdata->quest_flags[i] = -1;
    send( ch, "Qflag %d on %s set to Done.\n\r", i,
          victim == ch ? "yourself" : victim->descr->name );
  }
  else {
    send( ch, "Syntax: qflag <num> [U|A|D]\n\r" );
  }
  return;
}

void do_cflag( char_data* ch, char *argument )
{
  player_data* victim;
  wizard_data* imm  = wizard( ch );
  int i;

  victim = ( imm->player_edit == NULL ? (player_data*) ch : imm->player_edit );

  if( *argument == '\0' ) {
    page_title( ch, "Cflags of %s", victim->descr->name );
    for( i = 0; i < 32*MAX_CFLAG; i++ ) {
      page( ch, "%5d (%1c)", i,
        is_set( victim->pcdata->cflags, i ) ? '*' : ' ' );
      if( i%8 == 7 )
        page( ch, "\n\r" );
      }
    if( i%8 != 0 )
      page( ch, "\n\r" );
    return;
    }

  i = atoi( argument );

  if( i < 0 || i >= 32*MAX_CFLAG ) {
    send( ch, "Cflag number out of range.\n\r" );
    return;
    }

  if ( victim->pcdata != NULL ) {
     switch_bit( victim->pcdata->cflags, i );
 
     send( ch, "Cflag %d on %s set to %s.\n\r", i,
           victim == ch ? "yourself" : victim->descr->name,
           is_set( victim->pcdata->cflags, i ) ? "true" : "false" );
  }

  return;
} 


/*
 *   FILE ROUTINES
 */

   
void load_quests( void )
{
  quest_data*  quest;
  FILE*           fp;
  char        letter;
  int           vnum;

  fprintf( stderr, "Loading quests...\n\r" );

  if( ( fp = fopen( QUEST_FILE, "r" ) ) == NULL ) 
    panic( "Load_quests: file not found" );

  if( strcmp( fread_word( fp ), "#QUESTS" ) ) 
    panic( "Load_quests: header not found" );
 
  for( ; ; ) {
    if( ( letter = fread_letter( fp ) ) != '#' ) 
      panic( "Load_quests: # not found." );
   
    if( ( vnum = fread_number( fp ) ) == -1 )
      break;

    quest          = new quest_data;
    quest->vnum    = vnum;      
    quest->message = fread_string( fp, MEM_QUEST );
    quest->points  = fread_number( fp );

    fread_number( fp );

    quest_list[vnum] = quest;
    }

  fclose( fp );
  return;
}


void save_quests( )
{
  quest_data*  quest;
  FILE*           fp;
  int              i;

  if( ( fp = fopen( QUEST_FILE, "w" ) ) == NULL ) {
    bug( "Save_quest: fopen" );
    return;
    }

  fprintf( fp, "#QUESTS\n\n" );
 
  for( i = 0; i < MAX_QUEST; i++ ) 
    if( ( quest = quest_list[i] ) != NULL ) {
      fprintf( fp, "#%d\n", quest->vnum );
      fprintf( fp, "%s~\n", quest->message );
      fprintf( fp, "%d 0\n", quest->points  );
      }

  fprintf( fp, "#-1\n" );
  fclose( fp );

  return;
}




