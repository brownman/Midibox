
// If changed do:         make
// To create Makefile:    ruby ./extruby.rb

/** :rdoc:

alsa_midi.cpp

This file contains the 'global' Driver functions. Also it contains the libraries
initialization method that defines all other classes.

*/
#define DEBUG

// #define DUMP_API

#pragma implementation
#include "alsa_midi.h"
#include "alsa_seq.h"
#include "alsa_midi_event.h"
#include "alsa_midi_queue.h"
#include "alsa_midi_client.h"
#include "alsa_midi_port.h"
#include "alsa_midi++.h"
#include "alsa_port_subscription.h"
#include "alsa_remove.h"
#include "alsa_client_pool.h"
#include "alsa_system_info.h"
#include "alsa_midi_timer.h"
#include "alsa_query_subscribe.h"

#if defined(DUMP_API)
#define DUMP_STREAM stderr
#endif

#include <ruby/dl.h>
#include <alsa/asoundlib.h>

#if defined(DEBUG)
#include <signal.h>
#endif

VALUE alsaDriver, alsaMidiError;

/** call-seq: seq_open([name = 'default' [, streams = +SND_SEQ_OPEN_INPUT+ [, mode = 0]]]) -> AlsaSequencer_i

Open the ALSA sequencer.

Parameters
[name]    The sequencer's "name". This is not a name you make up for your own purposes;
          it has special significance to the ALSA library.
          Usually you need to pass "default" here. This is also the default.

[streams] The read/write mode of the sequencer. Can be one of three values:
          - +SND_SEQ_OPEN_OUTPUT+ - open the sequencer for output only
          - +SND_SEQ_OPEN_INPUT+ - open the sequencer for input only
          - +SND_SEQ_OPEN_DUPLEX+ - open the sequencer for output and input, the default

          _Note_:
          Internally, these are translated to +O_WRONLY+, +O_RDONLY+ and +O_RDWR+ respectively and used
          as the second argument to the C library open() call.

[mode] Optional modifier. Can be either 0 (default), or else +SND_SEQ_NONBLOCK+, which will make
       read/write operations non-blocking. This can also be set later using
       RRTS::Driver::AlsaSequencer_i#nonblock.
       The default is blocking mode.

Returns: an RRTS::Driver::AlsaSequencer_i instance. This instance must be kept and passed to most of
the other sequencer functions. The instance will *not* be automatically freed or closed.

Creates a new handle and opens a connection to the kernel sequencer interface.
After a client is created successfully, an event with +SND_SEQ_EVENT_CLIENT_START+
is broadcast to the announce port.

See also RRTS::Driver::AlsaSequencer_i#close.
*/
static VALUE
wrap_snd_seq_open(int argc, VALUE *v_params, VALUE v_alsamod)
{
  VALUE v_name, v_streams, v_mode;
  rb_scan_args(argc, v_params, "03", &v_name, &v_streams, &v_mode);
  const char *const name = NIL_P(v_name) ? "default" : StringValuePtr(v_name);
  const int streams = NIL_P(v_streams) ? SND_SEQ_OPEN_DUPLEX : NUM2INT(v_streams);
  const int mode = NIL_P(v_mode) ? 0 /*blocking mode*/ : BOOL2INT(v_mode);
  snd_seq_t * seq = 0;
#if defined(DUMP_API)
  fprintf(DUMP_STREAM, "snd_seq_open(null, %s, %d, %d)\n", name, streams, mode);
#endif
  const int r = snd_seq_open(&seq, name, streams, mode);
  if (r) RAISE_MIDI_ERROR("opening sequencer", r);
  return Data_Wrap_Struct(alsaSequencerClass, 0, 0, seq);
}

/** call-seq: client_info_malloc() -> AlsaClientInfo_i

Returns: an empty AlsaClientInfo_i instance.

The returned instance is automatically freed when the object goes out of scope.
*/
static VALUE
wrap_snd_seq_client_info_malloc(VALUE v_module)
{
#if defined(DUMP_API)
  fprintf(DUMP_STREAM, "snd_seq_client_info_malloc(null)\n");
#endif
  return Data_Wrap_Struct(alsaClientInfoClass, 0/*mark*/, snd_seq_client_info_free/*free*/,
                          XMALLOC(snd_seq_client_info));
}

/** call-seq: query_subscribe_malloc() -> AlsaQuerySubscribe_i

Returns: an empty AlsaQuerySubscribe_i instance.

The returned instance is automatically freed when the object goes out of scope.
*/
static VALUE
wrap_snd_seq_query_subscribe_malloc(VALUE v_module)
{
#if defined(DUMP_API)
  fprintf(DUMP_STREAM, "snd_seq_query_subscribe_malloc(null)\n");
#endif
  return Data_Wrap_Struct(alsaQuerySubscribeClass, 0/*mark*/, snd_seq_query_subscribe_free/*free*/,
                          XMALLOC(snd_seq_query_subscribe));
}

/** call-seq: system_info_malloc() -> AlsaSystemInfo_i

Returns: a new, empty AlsaSystemInfo_i instance which is automatically freed
*/
static VALUE wrap_snd_seq_system_info_malloc(VALUE v_module)
{
#if defined(DUMP_API)
  fprintf(DUMP_STREAM, "snd_seq_system_info_malloc(null)\n");
#endif
  return Data_Wrap_Struct(alsaSystemInfoClass, 0/*mark*/, snd_seq_system_info_free/*free*/,
                          XMALLOC(snd_seq_system_info));
}

/** call-seq: ev_malloc() -> AlsaMidiEvent_i

Returns: a new AlsaMidiEvent_i instance, which is automatically freed. The buffer comes
back uninitialized so you may want to call RRTS::Driver::AlsaMidiEvent_i#clear first.

This has no counterpart in the alsa API, since one would never (have to) do this.
*/
static VALUE
ev_malloc(VALUE v_module)
{
  return Data_Wrap_Struct(alsaMidiEventClass, 0/*mark*/, free/*free*/, ALLOC(snd_seq_event_t));
}

/** call-seq: queue_info_malloc() -> AlsaQueueInfo_i

Allocates and returns a new, empty queue_info structure which is automatically freed when the instance
goes out of scope
*/
static VALUE
wrap_snd_seq_queue_info_malloc(VALUE v_module)
{
#if defined(DUMP_API)
  fprintf(DUMP_STREAM, "snd_seq_queue_info_malloc(null)\n");
#endif
  return Data_Wrap_Struct(alsaQueueInfoClass, 0/*mark*/, snd_seq_queue_info_free/*free*/,
                          XMALLOC(snd_seq_queue_info));
}


/** call-seq: port_info_malloc() -> AlsaPortInfo_i

allocates an empty snd_seq_port_info_t instance using standard malloc

Returns: the port_info instance, it will be automatically freed
*/
static VALUE
wrap_snd_seq_port_info_malloc(VALUE v_module)
{
#if defined(DUMP_API)
  fprintf(DUMP_STREAM, "snd_seq_port_info_malloc(null)\n");
#endif
  return Data_Wrap_Struct(alsaPortInfoClass, 0/*mark*/, snd_seq_port_info_free,
                          XMALLOC(snd_seq_port_info));
}

/** call-seq: queue_tempo_malloc() -> AlsaQueueTempo_i

This method is not necessary since it is automatically called by RRTS::Driver::AlsaQueue_i#tempo, if no
AlsaQueueTempo_i instance is passed to it
*/
static VALUE
wrap_snd_seq_queue_tempo_malloc(VALUE v_module)
{
#if defined(DUMP_API)
  fprintf(DUMP_STREAM, "snd_seq_queue_tempo_malloc(null)\n");
#endif
  return Data_Wrap_Struct(alsaQueueTempoClass, 0/*mark*/, snd_seq_queue_tempo_free/*free*/,
                          XMALLOC(snd_seq_queue_tempo));
}

/** call-seq: queue_tempo_malloc() -> AlsaQueueStatus_i

Not required since done automatically called internally by RRTS::Driver;;AlsaQueue_i#status if
no instance is passed to it.
*/
static VALUE
wrap_snd_seq_queue_status_malloc(VALUE v_module)
{
#if defined(DUMP_API)
  fprintf(DUMP_STREAM, "snd_seq_queue_status_malloc(null)\n");
#endif
  return Data_Wrap_Struct(alsaQueueStatusClass, 0/*mark*/, snd_seq_queue_status_free/*free*/,
                          XMALLOC(snd_seq_queue_status));
}

/** call-seq: port_subscribe_malloc() -> AlsaPortSubscription_i
Returns: a new port_subscribe structure which will be freed automatically
*/
static VALUE
wrap_snd_seq_port_subscribe_malloc(VALUE v_mod)
{
#if defined(DUMP_API)
  fprintf(DUMP_STREAM, "snd_seq_port_subscribe_malloc(null)\n");
#endif
  return Data_Wrap_Struct(alsaPortSubscriptionClass, 0/*mark*/, snd_seq_port_subscribe_free/*free*/,
                          XMALLOC(snd_seq_port_subscribe));
}

/** call-seq:  remove_events_malloc() -> AlsaRemoveEvents_i

Returns a new RRTS::Driver::AlsaRemoveEvents_i instance, which will be freed automatically
*/
static VALUE
wrap_snd_seq_remove_events_malloc(VALUE v_mod)
{
#if defined(DUMP_API)
  fprintf(DUMP_STREAM, "snd_seq_remove_events_malloc(null)\n");
#endif
  return Data_Wrap_Struct(alsaRemoveEventsClass, 0/*mark*/, snd_seq_remove_events_free/*free*/,
                          XMALLOC(snd_seq_remove_events));
}

/** call-seq: client_pool_malloc() -> AlsaClientPool_i

Allocates a new client_pool structure, which is automatically freed
*/
static VALUE
wrap_snd_seq_client_pool_malloc(VALUE v_mod)
{
#if defined(DUMP_API)
  fprintf(DUMP_STREAM, "snd_seq_client_pool_malloc(null)\n");
#endif
  return Data_Wrap_Struct(alsaClientPoolClass, 0/*mark*/, snd_seq_client_pool_free/*free*/,
                          XMALLOC(snd_seq_client_pool));
}

/** call-seq: snd_strerror(errno) -> string

Returns: the errorstring for the given system- or alsa-error.
*/
static VALUE
wrap_snd_strerror(VALUE v_module, VALUE v_err)
{
//   fprintf(stderr, "snd_strerror(%d) -> %s\n", NUM2INT(v_err), snd_strerror(NUM2INT(v_err)));
  return rb_str_new2(snd_strerror(NUM2INT(v_err)));
}

VALUE param2sym(uint param)
{
  static const char *paramname[128] = {
    "bank", "modwheel", "breath", 0, "foot",
    "portamento_time", "data_entry", "volume", "balance", 0,
    // 10
    "pan", "expression", "effect1", "effect2", 0,
    0, "general_purpose1", "general_purpose2", "general_purpose3", "general_purpose4",
    // 20
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    // 30
    0, 0, "bank_lsb", "modwheel_lsb", "breath_lsb",
    0, "foot_lsb", "portamento_time_lsb", "data_entry_lsb", "main_volume_lsb",
    // 40
    "balance_lsb", 0, "pan_lsb", "expression_lsb", "effect1_lsb",
    "effect2_lsb", 0, 0, "general_purpose1_lsb", "general_purpose2_lsb",
    // 50
    "general_purpose3_lsb", "general_purpose4_lsb", 0, 0, 0,
    0, 0, 0, 0, 0,
    // 60
    0, 0, 0, 0, "sustain",
    "portamento", "sostenuto", "soft", "legato", "hold2",
    // 70
    "sound_variation", "timbre", "release", "attack", "brightness",
    "sc6", "sc7", "sc8", "sc9", "sc10",
    // 80 (0x50)
    "general_purpose5", "general_purpose6", "general_purpose7", "general_purpose8", "portamento_control",
     0, 0, 0, 0, 0,
     // 90
     0, "reverb", "tremolo", "chorus", "detune",
     "phaser", "data_increment", "data_decrement", "nonreg_parm_num_lsb", "nonreg_parm_num",
     // 100 (0x64)
     "regist_parm_num_lsb", "regist_parm_num", 0, 0, 0,
     0, 0, 0, 0, 0,
     // 110 (0x6d)
     0, 0, 0, 0, 0,
     0, 0, 0, 0, 0,
     // 120 (0x78)
     "all_sounds_off", "reset_controllers", "local_control_switch", "all_notes_pff", "omni_off",
     "omni_on", "mono", "poly" // ?
  };
  const char *const parmname = paramname[param];
  if (parmname)
    return ID2SYM(rb_intern(parmname));
  return INT2NUM(param);
}

/** call-seq: parse_address(arg) ->  [clientid, portid]

parse the given string and get the sequencer address

Parameters:
[arg] the string to be parsed

Returns: clientid + portid on success or it raises a AlsaMidiError.

This function parses the sequencer client and port numbers from the given string.
The client and port tokes are separated by either colon or period, e.g. 128:1.
The function accepts also a client name not only digit numbers.

The arguments could be '20:2' or 'MIDI2:0' etc.  Portnames are *not* understood!

See RRTS::Driver::AlsaSequencer_i#parse_address
*/
static VALUE
wrap_snd_seq_parse_address(VALUE v_module, VALUE v_arg)
{
  snd_seq_addr_t ret;
  const char *const arg = StringValuePtr(v_arg);
  const int r = snd_seq_parse_address(0, &ret, arg);
  if (r < 0) RAISE_MIDI_ERROR_FMT2("Invalid port '%s' - %s", arg, snd_strerror(r));
  return rb_ary_new3(2, INT2NUM(ret.client), INT2NUM(ret.port));
}

/** call-seq: param2sym(param) -> symbol

Parameters:
[param] if not already a symbol it is used as a controllerevent index and the correct
        symbol is returned.

Returns: if the passed parameter is an integer, the accompanying symbol, which is
the symbol of the 'param' event attribute, used for ControllerEvent.
Otherwise, return param as is.

Example:
    Driver::param2sym(0) -> :bank
*/
static VALUE
param2sym_v(VALUE v_module, VALUE v_param)
{
  if (FIXNUM_P(v_param))
    return param2sym(NUM2UINT(v_param));
  return v_param; // asume it is already a symbol then
}

#if defined(DEBUG)
struct rbtbr_data_block {
  int blocktime;
};

static VALUE
i_block_test(void *ptr)
{
  struct rbtbr_data_block &block = *(struct rbtbr_data_block *)ptr;
  sigset_t newmask;
  sigemptyset(&newmask);
  sigaddset(&newmask, SIGINT);
  pthread_sigmask(SIG_BLOCK, &newmask, 0);
  sleep(block.blocktime);
  pthread_sigmask(SIG_UNBLOCK, &newmask, 0);
  return Qnil;
}

/** call-seq:  block_test blocktime block

Parameters:
[blocktime]     blocktime in seconds
[block]  to be executed without GVL (Giant VM Lock)

The block is executed with the GVL temporarily released
*/
static VALUE
alsaDriver_block_test(VALUE v_driver, VALUE v_blocktime)
{
  // I don't trust ruby's 'sleep' method.  A real sleep that cannot be interrupted:
  struct rbtbr_data_block block = { NUM2INT(v_blocktime) };
  return rb_thread_blocking_region(i_block_test, &block, RUBY_UBF_PROCESS, 0);
}

/** call-seq: block_test_pure_sleep blocktime

Parameters:
[blocktime] blocktime in seconds

Just calls the C function sleep()
*/
static VALUE
alsaDriver_block_test_pure_sleep(VALUE v_driver, VALUE v_blocktime)
{
  sleep(NUM2INT(v_blocktime));
  return Qnil;
}

/** call-seq: block_test_ruby_sleep blocktime

Parameters:
[blocktime] blocktime in seconds

This method just calls Kernel#sleep
*/
static VALUE
alsaDriver_block_test_ruby_sleep(VALUE v_driver, VALUE v_blocktime)
{
  const ID id_sleep = rb_intern("sleep");
  rb_funcall(v_driver, id_sleep, 1, v_blocktime);
  return Qnil;
}

/** call-seq: Kernel#sleep_eintr_test

Added to kernel if compiled with DEBUG. If called it calls poll()
until an interrupt takes place.
*/
static VALUE
kernel_sleep_eintr_test(VALUE v_kernel)
{
  struct pollfd fd[0];
  fprintf(stderr, "Polling:");
  for (;; )
    {
      fprintf(stderr, ".");
      const int r = poll(fd, 0, 1000 /*ms*/);
      //rb_raise(alsaMidiError, "EINTR...."); // EXPERIMENTAL  NO PROBLEM
      if (r < 0)
        {
          if (errno == EINTR)
            RAISE_MIDI_ERROR_FMT0("EINTR....");
        }
      else
          break;
    }
  fprintf(stderr, "\n");
  return Qnil;
}
#endif

static VALUE error_handler_block_proc = Qnil;

static void
my_error_handler(const char *file, int line, const char *function, int err, const char *fmt, ...)
{
  va_list arg;
  char *buffer = 0;
  try
    {
      va_start(arg, fmt);
      if (vasprintf(&buffer, fmt, arg) < 0)
        {
          va_end(arg);
          RAISE_MIDI_ERROR_FMT1("Internal problem, could not vasprintf... errno=%d", errno);
        }
      va_end(arg);
      rb_funcall(error_handler_block_proc, rb_intern("call"), 5, rb_str_new2(file),
                  INT2NUM(line), rb_str_new2(function), INT2NUM(err), rb_str_new2(buffer));
    }
  catch (...)
    {
      free(buffer);
      throw;
    }
  free(buffer);
}

/** call-seq: snd_lib_error_set_handler do |file, line, funcname, errcode, errtext| ... end
I don't know when this is triggered but it seems a good idea to not use it.
Or at least to raising AlsaMidiError from it.
I assume it is called if snd_lib_error is called which occurs a lot in the overall Alsa code, but
rarely in the sequencer part (12 calls to SNDERR in total).

This function sets a new error handler, or (if handler is NULL) the default one which prints the error messages to stderr.

Note that the parameters differ from the C version as fmt + args is already formatted into errtext.

FIXME: it seems to work better if RAISE_MIDI_ERROR would be replaced with SNDERR, and that
we would always set a handler to raise a ruby exception.
However, does the alsa lib handle exceptions correctly?

*/
static VALUE
wrap_snd_lib_error_set_handler(VALUE v_driver)
{
  if (!rb_block_given_p())
    {
      snd_lib_error_set_handler(0);
      error_handler_block_proc = Qnil;
    }
  else
    {
      if (error_handler_block_proc == Qnil) // first call
        snd_lib_error_set_handler(my_error_handler);
      error_handler_block_proc = rb_block_proc();
    }
}

extern "C" void
Init_alsa_midi()
{
  rb_global_variable(&error_handler_block_proc);
  VALUE rrtsModule = rb_define_module("RRTS");
  /** Document-class: RRTS::Driver

    Driver is the namespace module for the basic Alsa MIDI API mapping.
    Among other things all constants from the Alsa API are stored here. These
    are not documented here. The original documentation is available on the net,
    see http://www.alsa-project.org/alsa-doc/alsa-lib/modules.html

    This module contains the basic sequencer constructor RRTS::Driver::seq_open
    and a few general allocation functions.
  */
  alsaDriver = rb_define_module_under(rrtsModule, "Driver");
  /** Document-class: RRTS::AlsaMidiError

      This is an exception class that inherits from StandardError.
      It is used for all RRTS::Driver errors.
  */
  alsaMidiError = rb_define_class_under(rrtsModule, "AlsaMidiError", rb_eStandardError);
  // class to store the result of snd_seq_port_subscribe_malloc: a snd_seq_port_subscribe_t*
  // alsaPortClass = rb_define_class_under(alsaDriver, "AlsaPort_i", rb_cObject);
  rb_define_module_function(alsaDriver, "seq_open", RUBY_METHOD_FUNC(wrap_snd_seq_open), -1);

  // since snd_seq_open is here
  WRAP_CONSTANT(SND_SEQ_OPEN_OUTPUT); // open the sequencer for output only
  WRAP_CONSTANT(SND_SEQ_OPEN_INPUT); //- open the sequencer for input only
  WRAP_CONSTANT(SND_SEQ_OPEN_DUPLEX); // - open the sequencer for output and input
  WRAP_CONSTANT(SND_SEQ_NONBLOCK); // - open the sequencer in non-blocking mode

  // all freed automatically.
  rb_define_module_function(alsaDriver, "client_info_malloc", RUBY_METHOD_FUNC(wrap_snd_seq_client_info_malloc), 0);
  rb_define_module_function(alsaDriver, "port_info_malloc", RUBY_METHOD_FUNC(wrap_snd_seq_port_info_malloc), 0);
  rb_define_module_function(alsaDriver, "queue_info_malloc", RUBY_METHOD_FUNC(wrap_snd_seq_queue_info_malloc), 0);
  rb_define_module_function(alsaDriver, "queue_tempo_malloc", RUBY_METHOD_FUNC(wrap_snd_seq_queue_tempo_malloc), 0);
  rb_define_module_function(alsaDriver, "queue_status_malloc", RUBY_METHOD_FUNC(wrap_snd_seq_queue_status_malloc), 0);
  // the snd_seq_port_subscribe_free is called automatically.
  rb_define_module_function(alsaDriver, "port_subscribe_malloc", RUBY_METHOD_FUNC(wrap_snd_seq_port_subscribe_malloc), 0);
  rb_define_module_function(alsaDriver, "remove_events_malloc", RUBY_METHOD_FUNC(wrap_snd_seq_remove_events_malloc), 0);
  rb_define_module_function(alsaDriver, "client_pool_malloc", RUBY_METHOD_FUNC(wrap_snd_seq_client_pool_malloc), 0);
  rb_define_module_function(alsaDriver, "system_info_malloc", RUBY_METHOD_FUNC(wrap_snd_seq_system_info_malloc), 0);
  rb_define_module_function(alsaDriver, "query_subscribe_malloc", RUBY_METHOD_FUNC(wrap_snd_seq_query_subscribe_malloc), 0);
  rb_define_module_function(alsaDriver, "ev_malloc", RUBY_METHOD_FUNC(ev_malloc), 0);
  rb_define_module_function(alsaDriver, "param2sym", RUBY_METHOD_FUNC(param2sym_v), 1);
  rb_define_module_function(alsaDriver, "snd_strerror", RUBY_METHOD_FUNC(wrap_snd_strerror), 1);
  rb_define_module_function(alsaDriver, "strerror", RUBY_METHOD_FUNC(wrap_snd_strerror), 1);
  // I only made this because aconnect uses it, but in fact this is stupid. Just catch the exceptions I guess.
  rb_define_module_function(alsaDriver, "snd_lib_error_set_handler", RUBY_METHOD_FUNC(wrap_snd_lib_error_set_handler), 0);
//   rb_define_module_function(alsaDriver, "snderr", RUBY_METHOD_FUNC(wrap_snderr), 1); DREADFULL SIN
  rb_define_module_function(alsaDriver, "parse_address", RUBY_METHOD_FUNC(wrap_snd_seq_parse_address), 1);

#if defined(DEBUG)
  rb_define_module_function(alsaDriver, "block_test", RUBY_METHOD_FUNC(alsaDriver_block_test), 1);
  rb_define_module_function(alsaDriver, "block_test_pure_sleep", RUBY_METHOD_FUNC(alsaDriver_block_test_pure_sleep), 1);
  rb_define_module_function(alsaDriver, "block_test_ruby_sleep", RUBY_METHOD_FUNC(alsaDriver_block_test_ruby_sleep), 1);
  rb_define_module_function(rb_mKernel, "sleep_eintr_test", RUBY_METHOD_FUNC(kernel_sleep_eintr_test), 0);
#endif

  alsa_seq_init();
  alsa_midi_queue_init();
  alsa_midi_client_init();
  alsa_midi_event_init();
  alsa_midi_port_init();
  port_subscription_init();
  alsa_remove_init();
  alsa_client_pool_init();
  alsa_system_info_init();
  alsa_midi_timer_init();
  alsa_query_subscribe_init();
  alsa_midi_plusplus_init();
}
