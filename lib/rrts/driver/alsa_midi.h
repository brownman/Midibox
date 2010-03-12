#if !defined(_RRTS_ALSA_MIDI_H)
#define _RRTS_ALSA_MIDI_H

#include <ruby.h>
#pragma interface

#define WRAP_CONSTANT(s) rb_define_const(alsaDriver, #s, INT2NUM(s))
#define WRAP_STRING_CONSTANT(s) rb_define_const(alsaDriver, #s, rb_str_new2(s))

extern VALUE alsaDriver, alsaMidiError;

#define RAISE_MIDI_ERROR_FMT3(fmt, a, b, c) rb_raise(alsaMidiError, fmt, a, b, c)
#define RAISE_MIDI_ERROR_FMT2(fmt, a, b) rb_raise(alsaMidiError, fmt, a, b)
#define RAISE_MIDI_ERROR_FMT1(fmt, a) rb_raise(alsaMidiError, fmt, a)
#define RAISE_MIDI_ERROR_FMT0(fmt) rb_raise(alsaMidiError, fmt)
#define RAISE_MIDI_ERROR(when, e) \
  RAISE_MIDI_ERROR_FMT3("%s failed with error %d: %s", when, e, snd_strerror(e))

static inline void rrts_deref(VALUE &v_val, const char *method)
{
//   fprintf(stderr, "%s:%d:rrts_deref(%p, %s)\n", __FILE__, __LINE__, &v_val, method);
  const ID id = rb_intern(method); // NEVER EVER static !!!!!!!!!!!!!!!!!!
//   fprintf(stderr, "rb_interned, id = %lu\n", id);
  if (rb_respond_to(v_val, id))
  {
//     fprintf(stderr, "responding, now calling!\n");
    v_val = rb_funcall(v_val, id, 0);
  }
//   fprintf(stderr, "ALIVE!\n");
}

static inline void rrts_deref_dirty(VALUE &v_val, const char *ivar)
{
  const ID id = rb_intern(ivar);
  if (RTEST(rb_ivar_defined(v_val, id)))
    v_val = rb_ivar_get(v_val, id);
}

// If ruby object v_val has method 'm' replace the whole thing with v_val.m
#define RRTS_DEREF(v_val, method) rrts_deref(v_val, #method)
/* If ruby object v_val has ivar 'varname' replace the whole thing with v_val.varname
Example: RRTS_DEREF_DIRTY(v_seq, @handle)

RRTS_DEREF_DIRTY is faster than RRTS_DEREF since we avoid the function call.
But it is slightly less flexible for the same reason
*/
#define RRTS_DEREF_DIRTY(v_val, varname) rrts_deref_dirty(v_val, #varname)


// If ruby object v_val has method 'm' replace the whole thing with v_val.m
#define RRTS_DEREF(v_val, method) rrts_deref(v_val, #method)

/* This class behaves similar to VALUE except that the Garbage Collector will
automatically leave it alone.
*/
class GCSafeValue
{
private:
  VALUE V;
public:
  GCSafeValue(VALUE v): V(v) { rb_gc_register_address(&V); }
  ~GCSafeValue() { rb_gc_unregister_address(&V); }
  VALUE operator ->() const { return V; }
  VALUE operator *() const { return V; }
  operator VALUE() const { return V; }
};

extern VALUE param2sym(uint param);

#if defined(TRACE)
#define trace(arg) fprintf(stderr, arg "\n");
#define trace1(arg, a) fprintf(stderr, arg "\n", a);
#define trace2(arg, a, b) fprintf(stderr, arg "\n", a, b);
#define trace3(arg, a, b, c) fprintf(stderr, arg "\n", a, b, c);
#define trace4(arg, a, b, c, d) fprintf(stderr, arg "\n", a, b, c, d);
#else
#define trace(arg)
#define trace1(arg, a)
#define trace2(arg, a, b)
#define trace3(arg, a, b, c)
#define trace4(arg, a, b, c, d)
#endif

#define INSPECT(x) RSTRING_PTR(rb_inspect(x))

#endif // _RRTS_ALSA_MIDI_H