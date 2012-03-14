#include <python2.7/Python.h>
#include "Transport.h"



typedef struct {
  PyObject_HEAD
  JackTPort	*tport;		/* the context holder */
} PyJackTPort;

static PyObject *tport_getattr(PyJackTPort *tp, char *name);
static PyObject *newTPort(PyObject *self, PyObject *args);
static PyObject *tport_stop(PyObject *self, PyObject *args);
static PyObject *tport_stop_recording(PyObject *self, PyObject *args);
static PyObject *tport_start(PyObject *self, PyObject *args);
static PyObject *tport_wait(PyObject *self, PyObject *args);
static PyObject *tport_got_signal(PyObject *self, PyObject *args);
static PyObject *tport_get_peaks(PyObject *self, PyObject *args);
static PyObject *tport_reset_peaks(PyObject *self, PyObject *args);
static void tport_dealloc(PyJackTPort *tport);
static PyObject *tport_getattr(PyJackTPort *self, char *args);

#define is_tport(v)		((v)->ob_type == &PyJackTPort)

static char* members[] = {
  NULL
};

static PyTypeObject JackTPortType = {
  PyObject_HEAD_INIT(NULL)
  0,				     /*ob_size*/
  "JackTPort",			     /*tp_name*/
  sizeof(PyJackTPort),		     /*tp_size*/
  0,				     /*tp_itemsize*/
  /* methods */
  (destructor)tport_dealloc,	     /*tp_dealloc*/
  0,				     /*tp_print*/
  (getattrfunc)tport_getattr,	     /*tp_getattr*/
  0,				     /*tp_setattr*/
  0,				     /*tp_compare*/
  0,				     /*tp_repr*/
};


static struct PyMethodDef tport_methods[] = {
  {"startRecording",	(PyCFunction)tport_start, 1},
  {"stopRecording",	(PyCFunction)tport_stop_recording, 1},
  {"stop",	(PyCFunction)tport_stop, 1},
  {"waitTillFinished",	(PyCFunction)tport_wait, 1},
  {"gotSignal",	(PyCFunction)tport_got_signal, 1},
  {"getPeaks",	(PyCFunction)tport_get_peaks, 1},
  {"resetPeaks",	(PyCFunction)tport_reset_peaks, 1},
  {NULL,		NULL}		/* sentinel */
};

static PyObject *tport_getattr(PyJackTPort *tp, char *name)
{
  if (strcmp(name, "__members__") == 0) {
    int i = 0;
    PyObject *list = NULL;

    while (members[i])
      i++;
    if (!(list = PyList_New(i)))
      return NULL;

    i = 0;
    while (members[i]) {
      PyObject* v = PyString_FromString(members[i]);
      if (!v || PyList_SetItem(list, i, v) < 0) {
        Py_DECREF(list);
        return NULL;
      }
      i++;
    }
    return list;
  }
  return Py_FindMethod(tport_methods, (PyObject *)tp, name);
}


static PyObject *newTPort(PyObject *self, PyObject *args)
{
  PyJackTPort *tport;
  unsigned int card, bps, sr;
    
  if (!PyArg_ParseTuple(args, "III", &card, &bps, &sr))
	return NULL;

  tport = (PyJackTPort *)PyObject_New(PyJackTPort, &JackTPortType);
  if (tport == NULL)
    return NULL;

  tport->tport = new JackTPort(bps, sr);
        
  return (PyObject *)tport;
}

static PyObject *tport_start(PyObject *self, PyObject *args)
{
  char *path;
  if (!PyArg_ParseTuple(args, "s", &path))
	return NULL;
  ((PyJackTPort*)self)->tport->startRecording(path);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *tport_stop(PyObject *self, PyObject *args)
{
  ((PyJackTPort*)self)->tport->stop();

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *tport_stop_recording(PyObject *self, PyObject *args)
{
  ((PyJackTPort*)self)->tport->stopRecording();

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *tport_wait(PyObject *self, PyObject *args)
{
  ((PyJackTPort*)self)->tport->wait();   
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *tport_got_signal(PyObject *self, PyObject *args)
{
  return  Py_BuildValue("i", ((PyJackTPort*)self)->tport->gotSignal());
}

static PyObject *tport_get_peaks(PyObject *self, PyObject *args)
{
  return  Py_BuildValue("(l,l)", 
                        ((PyJackTPort*)self)->tport->getmaxa(),  
                        ((PyJackTPort*)self)->tport->getmaxb());
}

static PyObject *tport_reset_peaks(PyObject *self, PyObject *args)
{
  ((PyJackTPort*)self)->tport->resetmax();
  Py_INCREF(Py_None);
  return Py_None;
}

static void tport_dealloc(PyJackTPort *tport)
{
  delete(tport->tport);
  PyObject_Del(tport);
}

static PyMethodDef moduleMethods[] = {
  {"newTPort",  newTPort, METH_VARARGS},
  {NULL,      NULL}        /* Sentinel */
};


extern "C" 
{
  void inittransport()
  {
	Py_InitModule("transport", moduleMethods);
  }
}

