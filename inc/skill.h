#include <math.h>

/*
 *   PRACTICES
 */


double  total_pracs  ( char_data* );

inline double expected_pracs( char_data* ch )
{
/*
  return 10+(ch->shdata->level)*(10+ch->Intelligence( )+ ch->Wisdom( ));

  return 4*(sqrt(ch->shdata->level*ch->shdata->level*ch->shdata->level))*
    ((ch->Intelligence( )+ch->Wisdom( ))/18);
*/
  return 10+4*ch->shdata->level*(10+ch->Intelligence( )+
    ch->Wisdom( ))/5;
}


/*
 *  TRAINER HEADER
 */


class Trainer_Data
{
 public:
  trainer_data*   next;
  room_data*      room;
  char_data*       mob;
  int          trainer;
  int            skill  [ MAX_TRAIN ];

  Trainer_Data( ) {
    record_new( sizeof( trainer_data ), MEM_TRAINER );
    vzero( skill, MAX_TRAIN );
    mob  = NULL;
    next = NULL;
    return;
    }  

  ~Trainer_Data( ) {
    record_delete( sizeof( trainer_data ), MEM_TRAINER );
    return;
    }
};


void   load_trainers    ( void );
void   save_trainers    ( void );
void   set_trainer      ( mob_data*, room_data* );












