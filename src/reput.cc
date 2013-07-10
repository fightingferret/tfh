#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "define.h"
#include "struct.h"


void   recruit_member        ( char_data *ch, char_data *member );
void   add_to_faction        ( char_data *ch, int faction );
void   remove_from_faction   ( char_data *ch, int faction );
char*  relation_string       ( int a, int b );

/* are_allied - used for checking introductions (if these are limited)
 * also used to see who can group with who.
 */

bool are_allied( char_data* c1, char_data* c2 )
{
  // Always allow grouping by race. (within light and within dark)
  bool c1_is_dark = (c1->shdata->race > RACE_LIZARD);
  bool c2_is_dark = (c2->shdata->race > RACE_LIZARD);
  if ( c1_is_dark == c2_is_dark )
     return TRUE;

  // Here's the new code.  Compare nation grievance arrays.
  int compatibility = 0;
  player_data* p1 = player(c1);
  player_data* p2 = player(c2);
  if ( p1 != NULL && p2 != NULL ) {
     for ( int i = NATION_SECOMBER; i < MAX_ENTRY_NATION; i++ ) {
        compatibility += p1->reputation.nation[i] * p2->reputation.nation[i];
     }
  }

  return (compatibility > 0);
}


bool associate( char_data* c1, char_data* c2 )
{
  if( are_allied( c1, c2 ) )
    return TRUE;

  bool c2_is_dark = (c2->shdata->race > RACE_LIZARD);
  send( c1, "You would never associate with such %s %s.\n\r", 
	c2_is_dark ? "an unsavory" : "a wholesome",
	race_table[ c2->shdata->race ].name );
      
  return FALSE;
}


/*
 *   MODIFY ROUTINE
 */


void modify_reputation( char_data*, char_data*, int )
{
  /*
  char             tmp  [ MAX_STRING_LENGTH ];
  pfile_data*    pfile;
  int         vic_clan  = victim->shdata->clan;
  int          ch_clan  = ch->shdata->clan;
  int              mod;
  int                i;
  
  if( is_set( &ch->in_room->room_flags, RFLAG_ARENA ) ) 
    return;
 
  if( ch->species != NULL || vic_clan < 2 || victim == ch )
    return;

  pfile = ch->pcdata->pfile;

  if( type == REP_STOLE_FROM ) {
    pfile->reputation[vic_clan-2] += IS_NPC( victim ) ? -100 : -200;
    send( ch, "Your reputation changes for stealing from %s.\n\r",
      victim->Seen_Name( ch ) ); 
    }

  if( type == REP_ATTACKED ) {
    pfile->reputation[vic_clan-2] += IS_NPC( victim ) ? -100 : -200; 
    send( ch, "Your reputation changes for attacking %s.\n\r",
      victim->Seen_Name( ch ) ); 
    }
  
  if( type == REP_KILLED ) {
    for( i = 2; i < MAX_ENTRY_CLAN; i++ ) {
      mod = ( victim->species == NULL ? -300 : -500 );
      if( i != vic_clan )
        mod /= 3;
      mod = mod*(clan_table[vic_clan].relation[i-2]-50)/100;
      pfile->reputation[i-2] += mod;
      }
    send( ch, "Your reputation changes for killing %s.\n\r",
      victim->Seen_Name( ch ) ); 
    }

  if( ch_clan >= 2 && ch_clan < MAX_ENTRY_CLAN
    && pfile->reputation[ch_clan-2] < 0 ) {
    send( ch, "%sYou are cast out of your clan!!%s\n\r",
      bold_v( ch ), normal( ch ) );  
    sprintf( tmp, "%s is cast out of the %s clan for fighting %s.",
      ch->descr->name, clan_table[ch_clan].name, victim->descr->name );
    info( tmp, LEVEL_BUILDER, tmp, IFLAG_CLANS, 1, ch );
    ch->shdata->clan = CLAN_NONE;
    }
    */
  return;
}


/*
 *   REPUTATION COMMAND
 */

 
index_data reputation_index [] =
{ 
  { "abhorred",  "",   -600 },
  { "despised",  "",   -400 },
  { "hated",     "",   -200 },
  { "wanted",    "",    -50 },
  { "suspect",   "",      0 },
  { "dubious",   "",     50 },
  { "accepted",  "",    200 },
  { "credible",  "",    400 },
  { "reputable", "",    600 },
  { "esteemed",  "",     -1 },
};


void do_reputation( char_data* ch, char* argument )
{
  player_data*     pc  = (player_data*) ch;
  char_data*   victim;

  if( is_mob( ch ) )
    return;

  if( ch->shdata->level >= LEVEL_APPRENTICE && *argument != '\0' ) {
    if( ( victim = one_character( ch, argument, "show reputation",
      ch->array, (thing_array*) &player_list ) ) == NULL )
      return;
    if( ( pc = player( victim ) ) == NULL ) {
      send( ch, "Reputation cannot act on npcs.\n\r" );
      return;
      }
    }

  page_underlined( ch, "Sacrifices\n\r" );
  page( ch, "%17s: %d\n\r", "Cp", pc->reputation.gold );
  page( ch, "%17s: %d\n\r", "Blood", pc->reputation.blood );
  page( ch, "%17s: %d\n\r", "Magic", pc->reputation.magic );
  page( ch, "\n\r" );

  page_underlined( ch, "Nation Grievances\n\r" );
  for( int i = 1; i < MAX_ENTRY_NATION; i++ ) 
    page( ch, "%17s: %d\n\r", nation_table[i].name,
      pc->reputation.nation[i] );

  int total = 0;

  for( int i = 0; i < MAX_ENTRY_ALIGNMENT; i++ ) 
   total += pc->reputation.alignment[i];

  if( total == 0 )
    return;

  page( ch, "\n\r" );
  page_underlined( ch, "Alignment Exp (%)\n\r" );

  for( int i = 0; i < MAX_ENTRY_ALIGNMENT; i++ ) 
    page( ch, "%17s: %6.2f\n\r", alignment_table[i].name,
      (float) 100*pc->reputation.alignment[i]/total );
}


/*
 *   WANTED COMMAND
 */


void do_wanted( char_data* ch, char* )
{
  if( is_confused_pet( ch ) )
    return;

  send( "Command is disabled.\n\r", ch );
  return;

  /*
  char   buf  [ MAX_STRING_LENGTH ];
  int   clan  = ch->shdata->clan;
  int  count;
  int      i; 

  if( argument[0] != '\0' ) {
    for( clan = CLAN_IMMORTAL+1; clan < MAX_ENTRY_CLAN; clan++ ) 
      if( !strncasecmp( argument, clan_table[clan].name,
        strlen( argument ) ) )
        break;

    if( clan == MAX_ENTRY_CLAN ) {
      send( ch, "That clan is unknown.\n\r" );
      return;
      }
    }

  if( clan >= MAX_ENTRY_CLAN ) {
    send( ch, "Your clan does not have a wanted list.\n\r" );
    return;
    }

  count = 0;
  page_title( ch, "WANTED" );
  sprintf( buf, "%24sName%7sBounty  Kills\n\r", " ", " " );
  page( ch, buf );

  for( i = 0; i < max_pfile; i++ ) {
    if( pfile_list[i] != NULL && pfile_list[i]->reputation[clan] < 0 ) {
      sprintf( buf, "%24s%-10s  %-7d  %-5d\r\n", "", pfile_list[i]->name,
        -pfile_list[i]->reputation[clan], 0 );
      page( ch, buf );
      count++;
      }
    }

  if( count == 0 )
    page(
      "                        Wanted list is currently empty.\n\r", ch );
      */
  return;
}

/*
 *   RELATIONS COMMAND
 */


char* relation_string( int rel )
{
  if( rel <= 100 && rel > 66 )  return "+++";
  else if( rel > 33 )   return " ++";
  else if( rel > 0 )    return " + ";
  else if( rel == 0 )   return " = ";
  else if( rel > -33 )  return " - ";
  else if( rel > -66 )  return " --";
  else if( rel > -101 ) return "---";

  return "???"; // outside range [-100,100]
}


void do_relations( char_data* ch, char* argument )
{
  char   tmp  [ MAX_STRING_LENGTH ];
  char tmp2[ONE_LINE];

  // Parse argument flags
  int flags;
  if ( !get_flags( ch, argument, &flags, "n", "relations" ) )
     return ;

  // When doing a 'relat -n', the columns must all be 5 characters wide
  // instead of 4, so the -100's will fit.
  bool show_numeric = is_set(&flags, 0) && is_apprentice(ch);

  // Display Nation vs Nation relations in a matrix
  page_title( ch, "Nation vs Nation" );

  page( ch, "\n\r%16s", "" );
  tmp[0] = '\0';
  for( int i = NATION_SECOMBER; i < MAX_ENTRY_NATION; i++ ) {
     if ( show_numeric ) {       // make columns 5 chars wide
        strcat( tmp, " " );
     }
     strcat( tmp, nation_table[i].abbrev );
     strcat( tmp, " " );
  }
  strcat( tmp, "\n\r" );
  page( ch, tmp );

  for( int i = NATION_SECOMBER; i < MAX_ENTRY_NATION; i++ ) {
     tmp[0] = '\0';
     sprintf( tmp, "      %-10.10s", nation_table[i].name );
     for( int j = NATION_SECOMBER; j < MAX_ENTRY_NATION; j++ )  {
        if ( show_numeric ) {       // make columns 5 chars wide
           sprintf( tmp2, "%4d ", nation_table[i].nation[j] ) ;
           strcat( tmp, tmp2 );
	}
	else { // columns normally 4 chars wide
           strcat( tmp, relation_string( nation_table[i].nation[j] ) );
           strcat( tmp, " " );
	}
     }
     strcat( tmp, "\n\r" );
     page( ch, tmp );
  }
  page( ch, "\n\r" );

  // Display Nation vs Alignment relations in a matrix
  page_title( ch, "Nation vs Alignment" );

  page( ch, "\n\r%24s", "" );
  tmp[0] = '\0';
  for( int i = 0; i < MAX_ENTRY_ALIGNMENT; i++ ) {
     if ( show_numeric ) {       // make columns 5 chars wide
        strcat( tmp, " " );
     }
    strcat( tmp, alignment_table[i].abbrev ); 
    strcat( tmp, "  " );
  }
  strcat( tmp, "\n\r" );
  page( ch, tmp );

  for( int i = NATION_SECOMBER; i < MAX_ENTRY_NATION; i++ ) {
    page( ch, "%19s%s  ", "", nation_table[i].abbrev );
    for( int j = 0; j < MAX_ENTRY_ALIGNMENT; j++ ) {
      if ( show_numeric ) {       // make columns 5 chars wide
         page( ch, "%4d%s", nation_table[i].alignment[j],
               j != MAX_ENTRY_ALIGNMENT-1 ? " " : "\n\r" );
      }
      else { // make columns 4 chars wide
         page( ch, "%3s%s", relation_string( nation_table[i].alignment[j] ),
               j != MAX_ENTRY_ALIGNMENT-1 ? " " : "\n\r" );
      }
    }
  }

  page( ch, "\n\r" );
  page_centered( ch, "[ See help relations for explanation of table ]" );
}


