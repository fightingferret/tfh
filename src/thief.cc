#include <sys/types.h>
#include <stdio.h>
#include "define.h"
#include "struct.h"

bool can_hide( char_data*, bool );

void 
do_gouge( char_data* ch, char* argument )
{
  char_data* victim;
  int  delay;

  if( is_confused_pet( ch ) )
    return;
  
  if( is_mob( ch ) )
    return;

  if( is_set( ch->pcdata->pfile->flags, PLR_PARRY ) ) {
    send( ch, "You can not gouge with parry on.\n\r" );
    return;
    }
    
  if( ch->get_skill( SKILL_EYE_GOUGE )  < 1 ) {
    send( ch, "You do not know how to gouge eyes.\n\r" );
    return;
    }

  if( (victim = get_victim( ch, argument, "Gouge whose eyes?\n\r" )) == NULL )
    return;

  if( victim == ch ) {
    send( ch, "Gouging your own eyes is not very productive.\n\r" );
    return;
  }

  if( victim->species != NULL 
    && !is_set( &victim->species->act_flags, ACT_HAS_EYES ) ) {
    send( ch,  "This creature has no eyes and cannot be blinded.\n\r" );
    return;
  }

  /*
  if ( is_set( victim->affected_by, AFF_BLIND ) ) {
    send( ch, "%s is already blinded!\n\r", victim );
    return;
  }
  */
 
  if( victim->fighting != ch && ch->fighting != victim ) {
    if( !can_kill( ch, victim ) )
      return;
    check_killer( ch, victim );
    }


  delay = gouge_attack( ch, victim );

  ch->fighting = victim ;
  react_attack( ch, victim );
  add_queue( &ch->active, delay );

  remove_bit( &ch->status, STAT_LEAPING );
  remove_bit( &ch->status, STAT_WIMPY );

  ch->improve_skill( SKILL_EYE_GOUGE );
  return;

}


int 
gouge_attack( char_data* ch, char_data* victim )
{
  int roll = number_range( 0, 20 )
            + ch->get_skill( SKILL_EYE_GOUGE )/2
            + (ch->shdata->dexterity - victim->shdata->dexterity)/2
            + (ch->shdata->level - victim->shdata->level)/4;

  if( roll < 6 ) {
      send( ch, "You attempt to gouge %s but are unsuccessful.\n\r", victim );
      send( victim, "%s attempts to gouge you but is unsuccessful.\n\r", ch );
      send( *ch->array, 
           "%s attempts to gouge %s but is unsuccessful.\n\r",
           ch, victim );
    return 32;
  }

  if( roll > 20 ) {
    send( ch, "You gouge %s in the eye!!\n\r", victim );
    send( victim, "%s gouges you in the eye!!\n\r", ch );
    send( *ch->array, "%s gouges %s in the eye!!\n\r", ch, victim );

    // The blindness duration is in PULSE_TICK's (approx. 50 secs)
    int duration = 1 + number_range( 0, ch->get_skill(SKILL_EYE_GOUGE)/4 );
    spell_affect( ch, victim, ch->get_skill(SKILL_EYE_GOUGE), duration, 
		  SPELL_BLIND, AFF_BLIND );

    disrupt_spell( victim ); 
    set_delay( victim, 32 );
    return 20;
  }

  send( ch, "You attempt to gouge %s but fail.\n\r", victim );
  send( victim, "%s attempts to gouge you but fails.\n\r", ch );
  send( *ch->array, "%s attempts to gouge %s but fails.\n\r", ch, victim );
  return 20;
}

void 
do_disguise( char_data*, char* )
{
  return;
}


void 
do_garrote( char_data* ch, char* argument )
{
  int level = ch->get_skill( SKILL_GARROTE );
  obj_data* garrote = NULL;
  for ( int i = 0; i < ch->contents.size; i++ ) 
  {
     obj_data* an_object = object( ch->contents[i] );
     if ( an_object != NULL &&
          an_object->pIndexData->item_type == ITEM_GARROTE ) 
     {
	garrote = an_object;
        break;
     }
  }

  if( garrote == NULL ) {
    send( ch, "You don't have a garrote.\n\r" );
    return;
    }

  if( *argument == '\0' ) {
    send( ch, "You have no target for your garrote.\n\b");
    return;
    }
  
  if( level == 0 ) {
    send( "Garrote is not part of your repertoire\n\r", ch );
    return;
    }

  char_data* victim;
  if ( (victim = get_victim( ch, argument, "Garrote whom?\n\r" )) == NULL )
    return;

  if ( victim == ch ) {
    send( ch, "Garroting yourself is not very productive.\n\r" );
    return;
  }

  if( !can_kill( ch, victim ) )
    return;
  
  set_bit( victim->affected_by, AFF_CHOKING );
  send( victim, "A garrote begins to tighten around your neck.\n\r" );
  send( *victim->array, "The garrote tightens around %s neck.\n\r", victim ); 
  
  ch->fighting = victim ;
  react_attack( ch, victim );
  add_queue( &ch->active, 20 );

  remove_bit( &ch->status, STAT_WIMPY );
  remove_bit( &ch->status, STAT_LEAPING );

  set_delay( victim, 32 );
  ch->improve_skill( SKILL_GARROTE );
 
  attack( ch, victim, "garrote", garrote, -1, 0 );
  
  return;
}


/* 
 *   BACKSTAB FUNCTIONS
 */


void 
do_backstab( char_data* ch, char* argument )
{
  char_data*  victim;
  obj_data*      obj;
  int          skill  = ch->get_skill( SKILL_BACKSTAB );

  if( ch->mount != NULL ) {
    send( ch, "Backstabbing while mounted is beyond your skill.\n\r" );
    return;
    }

  if( *argument == '\0' ) {
    send( ch, "Backstab whom?\n\r" );
    return;
    }

  if( ( victim = one_character( ch, argument, "backstab", ch->array ) ) 
                                == NULL ) 
    return;

  if( opponent( ch ) != NULL ) {
    send( "You are already fighting someone.\n\r", ch );
    return;
    }

  if( victim == ch ) {
    send( "How can you sneak up on yourself?\n\r", ch );
    return;
    }

  if( skill == 0 ) {
    send( "Backstabbing is not part of your repertoire.\n\r", ch );
    return;
    }

  if( (obj = ch->Wearing(WEAR_HELD_R)) == NULL ) {
    send( "You need to be wielding a weapon to backstab.\n\r", ch );
    return;
    }

  if( !is_set( obj->pIndexData->extra_flags, OFLAG_BACKSTAB ) ) {
    send( ch, "It isn't possible to use %s to backstab.\n\r", obj );
    return;
    }

  if( opponent( ch ) != NULL ) {
    send( ch, "You are unable to backstab while fighting someone.\n\r" );
    return;
    }

  if( ch->Seen( victim ) && includes( victim->aggressive, ch ) ) {
    send( ch, "%s is too wary of you for backstab to succeed.\n\r", victim );
    return;
    }

  if( !can_kill( ch, victim ) )
    return;

  check_killer( ch, victim );
  ch->fighting = victim ;
  react_attack( ch, victim );
  add_queue( &ch->active, 20 );

  remove_bit( &ch->status, STAT_WIMPY );
  remove_bit( &ch->status, STAT_LEAPING );

  if ( !attack( ch, victim, "backstab", obj, -1, 0 ) ) {
    send( ch, "Your attempted backstab misses the mark.\n\r" );
  }

  return;
}


/*
 *   STEAL ROUTINES
 */


void 
do_steal( char_data* ch, char* argument )
{
  char           buf  [ MAX_INPUT_LENGTH ];
  char           arg  [ MAX_INPUT_LENGTH ];
  char_data*  victim;
  obj_data*      obj;

  if( is_confused_pet( ch ) )
    return;
 
  if( is_mob( ch ) )
    return;
 
  if( is_set( ch->pcdata->pfile->flags, PLR_PARRY ) ) {
    send( ch, "You can not steal with parry on.\n\r" );
    return;
    }

  argument = one_argument( argument, arg );

  for( ; ; ) {
    argument = one_argument( argument, buf );
    if( buf[ 0 ] == '\0' || !strcasecmp( buf, "from" ) )
      break;
    sprintf( arg+strlen( arg ), " %s", buf );
    }

  if( arg[0] == '\0' || argument[0] == '\0' ) {
    send( "Syntax: steal <object> from <character>\n\r", ch );
    return;
    }

  if( ( victim = one_character( ch, argument, "steal", ch->array ) ) == NULL ) 
    return;

  if( victim == ch ) {
    send( "That's pointless.\n\r", ch );
    return;
    }

  if( !can_kill( ch, victim ) ) {
    send( "You can't steal from them.\n\r", ch );
    return; 
    } 

  remove_bit( ch->pcdata->pfile->flags, PLR_PARRY );

  if( !ch->check_skill( SKILL_STEAL )
    || number_range( 3, 35 ) < victim->Intelligence( ) ) {
    leave_shadows( ch );
    ch->fighting = victim ;
    react_attack( ch, victim );
    add_queue( &ch->active, number_range(5,20) );
    set_delay( ch, 32 );
 
    remove_bit( &ch->status, STAT_LEAPING );
    remove_bit( &ch->status, STAT_WIMPY );
    send( victim, "%s tried to steal from you.\n\r", ch );
    send( *ch->array, "%s tried to steal from %s.\n\r", ch, victim );
    modify_reputation( ch, victim, REP_STOLE_FROM );
    if( victim->pShop != NULL ) {
      sprintf( buf, "Guards! %s is a thief.", ch->Name( victim ) );
      buf[8] = toupper(buf[8]); // capitalize thief's appearance or name
      do_yell( victim, buf );
      summon_help( victim, ch );
      }
    return;
  }

  if ( ( obj = one_object( victim, arg, "steal", &victim->contents ) ) 
                         == NULL ) {
    send( "You can't find it.\n\r", ch );
    return;
  }
    
  if ( !obj->droppable() ) {
    send( ch, "You can't pry it away.\n\r" );
    return;
  }

  if ( ch->contents.number >= ch->can_carry_n() ) {
    send( "You have your hands full.\n\r", ch );
    return;
  }

  if ( obj->Weight() > ch->Capacity() ) {
    send( "You can't carry that much weight.\n\r", ch );
    return;
  }

  thing_data* stolen_thing = obj->From( 1 );
  obj_data* stolen_obj = object(stolen_thing);
  if ( stolen_obj != NULL ) {
     stolen_obj->To( &ch->contents );
     send( ch, "You succeeded in stealing %s.\n\r", obj );
     ch->improve_skill( SKILL_STEAL );
  }

  return;
}


void do_heist( char_data* ch, char* argument )
{
/*
  char           buf  [ MAX_INPUT_LENGTH ];
  char           arg  [ MAX_INPUT_LENGTH ];
  char_data*  victim;
  obj_data*      obj;

  if( is_confused_pet( ch ) )
    return;
 
  if( is_mob( ch ) )
    return;
 
  if( is_set( ch->pcdata->pfile->flags, PLR_PARRY ) ) {
    send( ch, "You can not steal with parry on.\n\r" );
    return;
    }

  argument = one_argument( argument, arg );

  for( ; ; ) {
    argument = one_argument( argument, buf );
    if( buf[ 0 ] == '\0' || !strcasecmp( buf, "from" ) )
      break;
    sprintf( arg+strlen( arg ), " %s", buf );
    }

  if( arg[0] == '\0' || argument[0] == '\0' ) {
    send( "Syntax: steal <object> from <character>\n\r", ch );
    return;
    }

  if( ( victim = one_character( ch, argument, "steal", ch->array ) ) == NULL ) 
    return;

  if( victim == ch ) {
    send( "That's pointless.\n\r", ch );
    return;
    }

  if( !can_kill( ch, victim ) ) {
    send( "You can't steal from them.\n\r", ch );
    return; 
    } 

  remove_bit( ch->pcdata->pfile->flags, PLR_PARRY );

  if( !ch->check_skill( SKILL_STEAL )
    || number_range( 3, 35 ) < victim->Intelligence( ) ) {
    leave_shadows( ch );
    ch->fighting = victim ;
    react_attack( ch, victim );
    add_queue( &ch->active, number_range(5,20) );
    set_delay( ch, 32 );
 
    remove_bit( &ch->status, STAT_LEAPING );
    remove_bit( &ch->status, STAT_WIMPY );
    send( victim, "%s tried to steal from you.\n\r", ch );
    send( *ch->array, "%s tried to steal from %s.\n\r", ch, victim );
    modify_reputation( ch, victim, REP_STOLE_FROM );
    if( victim->pShop != NULL ) {
      sprintf( buf, "Guards! %s is a thief.", ch->Name( victim ) );
      buf[8] = toupper(buf[8]);
      do_yell( victim, buf );
      summon_help( victim, ch );
      }
    return;
    }

  if( ( obj = one_object( victim, arg, "steal", &victim->contents ) ) 
			 == NULL ) {
    send( "You can't find it.\n\r", ch );
    return;
    }
    
  if( !obj->droppable( ) ) {
    send( ch, "You can't pry it away.\n\r" );
    return;
    }

  if( ch->num_ins >= ch->can_carry_n( ) ) {
    send( "You have your hands full.\n\r", ch );
    return;
    }

  if( ch->wght_ins + obj.weight > ch->can_carry_w( ) ) {
    send( "You can't carry that much weight.\n\r", ch );
    return;
    }

  obj = remove( obj, 1 );
  put_obj( obj, ch );
  send( ch, "You succeeded in stealing %s.\n\r", obj );
  ch->improve_skill( SKILL_STEAL );
  return;
*/
}

 
/*
 *   SNEAK FUNCTIONS
 */


void do_sneak( char_data* ch, char* argument )
{
  if( not_player( ch ) ) {
    // Mobs with the sneak ability should be able to sneak.
    if ( ch->species != NULL &&
	 is_set( ch->species->affected_by, AFF_SNEAK ) ) {
       switch_bit( ch->affected_by, AFF_SNEAK );
    }
    return;
  }

  if( ch->shdata->skill[ SKILL_SNEAK ] == UNLEARNT ) {
    send( ch, "Sneaking is not something you are adept at.\n\r" );
    return;
  }

  if( !can_hide( ch, TRUE ) )
    return;

  if( toggle( ch, argument, "Sneak", ch->pcdata->pfile->flags, PLR_SNEAK ) ) {
    return;
  }

  send( ch, "Usage: sneak <on|off>\n\r" );
  return;
}


/*
 *   HIDE ROUTINES
 */


bool can_hide( char_data* ch, bool msg )
{
  room_data*  room  = ch->in_room;

  if( ch->mount != NULL ) {
    if( msg ) 
      send( ch, "Hiding while mounted is impossible.\n\r" );
    return FALSE;
  }

  if( ch->fighting != NULL ) {
    if( msg )
      send( ch, "Hiding while fighting someone is difficult.\n\r" );
    return FALSE;
  }

  if ( !ch->Can_See() ) {
    if( msg )
      send( ch, "Hiding while blind is extremely difficult.\n\r" );
    return FALSE;
  }

  if( is_set( ch->affected_by, AFF_FIRE_SHIELD ) 
    || is_set( ch->affected_by, AFF_ION_SHIELD )
    || is_set( ch->affected_by, AFF_FAERIE_FIRE ) ) {
    if( msg ) 
      send( ch, "Your fiery glow rather spoils that.\n\r" );
    return FALSE;
  }

  if( water_logged( room ) ) {
    if( msg )
      send( ch, "Hiding is unsoluble.\n\r" );
    return FALSE;
    }
    
  return TRUE;
}


void do_camouflage( char_data* ch, char* )
{
  /*
  char_data* rch;

  if( not_player( ch ) )
    return;

  if( ch->shdata->skill[ SKILL_CAMOUFLAGE ] == 0 ) {
    send( ch, "Camouflage is not something you are adept at.\n\r" );
    return;
    }

  if( leave_camouflage( ch ) )
    return;

  if( !can_hide( ch, TRUE ) )
    return;

  for( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room )
    if( rch != ch && ch->Seen( rch ) )
      add( ch->seen_by, rch );

  send( ch, "You camouflage yourself and disappear from plain view.\n\r" );
  fsend_seen( ch, "%s tries to blend in with %s surroundings.\n\r", ch,
    ch->His_Her( )  );
  
  set_bit( &ch->status, STAT_CAMOUFLAGED );
  set_bit( ch->affected_by, AFF_CAMOUFLAGE );

  ch->improve_skill( SKILL_CAMOUFLAGE );
  */

  return;
}


void do_hide( char_data* ch, char* )
{
  char_data* rch;

  if( not_player( ch ) )
    return;

  if( ch->shdata->skill[ SKILL_HIDE ] == 0 ) {
    send( ch, "Hiding is not something you are adept at.\n\r" );
    return;
    }

  if( leave_shadows( ch ) )
    return;

  if( !can_hide( ch, TRUE ) )
    return;

  for( int i = 0; i < *ch->array; i++ ) 
    if( ( rch = character( ch->array->list[i] ) ) != NULL
      && rch != ch && ch->Seen( rch ) )
      ch->seen_by += rch;

  send( ch, "You step into the shadows.\n\r" );
  fsend_seen( ch, "%s steps into the shadows attempting to hide.\n\r", ch );
  
  set_bit( &ch->status, STAT_HIDING );
  set_bit( ch->affected_by, AFF_HIDE );

  ch->improve_skill( SKILL_HIDE );
}


bool leave_camouflage( char_data* ch )
{
  if( !is_set( &ch->status, STAT_CAMOUFLAGED ) 
    && !is_set( ch->affected_by, AFF_CAMOUFLAGE ) ) 
    return FALSE;

  remove_bit( &ch->status, STAT_CAMOUFLAGED );
  remove_bit( ch->affected_by, AFF_CAMOUFLAGE );

  clear( ch->seen_by );

  send( ch, "You stop camouflaging yourself.\n\r" );
  send_seen( ch, "%s suddenly appears from nowhere.\n\r", ch );

  return TRUE;
}


bool leave_shadows( char_data* ch )
{
  if( !is_set( &ch->status, STAT_HIDING ) )
    return FALSE;

  remove_bit( &ch->status, STAT_HIDING );
  remove_bit( ch->affected_by, AFF_HIDE );

  clear( ch->seen_by );

  send( ch, "You stop hiding.\n\r" );
  send_seen( ch, "%s steps from the shadows.\n\r", ch );

  // What do room denizens do? 
  char_data* rch = NULL;
  if ( ch->array != NULL ) {
     for ( int i = 0; i < ch->array->size; i++ ) {
        rch = character( ch->array->list[i] );
        // If aggressive, they leap to attack.
	if ( rch != NULL && rch != ch && ch->Seen( rch ) &&
             is_aggressive( rch, ch ) ) {
	   init_attack( rch, ch );
	}
     }
  }

  return TRUE;
}


/* 
 *   DIP ROUTINE
 */


void do_dip( char_data* ch, char* argument )
{
  char              arg  [ MAX_INPUT_LENGTH ];
  obj_data*   container;
  obj_data*         obj;
  int             value;
  affect_data    affect;
  int             spell;

  if( !two_argument( argument, "from", arg ) ) {
    send( ch, "Syntax: Dip <object> [into] <object>\n\r" );
    return;
    }

  if( ( obj = one_object( ch, arg, "dip", &ch->contents ) ) == NULL ) 
    return;

  if( ( container = one_object( ch, argument, "dip into",
    &ch->contents, ch->array ) ) == NULL ) 
    return;

  if( container->pIndexData->item_type != ITEM_DRINK_CON
    && container->pIndexData->item_type != ITEM_FOUNTAIN ) {
    send( ch, "%s isn't something you can dip things into.\n\r", container );
    return;
    }

  if( container == obj ) {
    send( ch, "You can't dip %s into itself.\n\r", obj );
    return;
    }

  value               = container->value[1];
  container->value[1] = -2;

  if( value == 0 ) {
    send( ch, "%s is empty.\n\r", container );
    container->value[1] = value;
    return;
    }

  if( strip_affect( obj, AFF_BURNING ) ) {
    fsend( ch, "You extinguish %s by quickly dipping it into %s.",
      obj, container );
    fsend( *ch->array,
      "%s extinguishes %s by quickly dipping it into %s.",
      ch, obj, container );
    return;
    } 

  send( ch, "You dip %s into %s.\n\r", obj, container );
  send_seen( ch, "%s dips %s into %s.\n\r", ch, obj, container );
  container->value[1] = ( value == -1 ? -1 : max( 0, value-5 ) );

  if( ( spell = liquid_table[container->value[2]].spell ) == -1 ) 
    return;

  if( spell < SPELL_FIRST || spell >= WEAPON_FIRST ) {
    bug( "Do_dip: Liquid with non-spell skill." );
    return;
    }

  ( *spell_table[spell-SPELL_FIRST].function )( ch, NULL, obj, 10, -3 ); 

  return;
}  





