#ifndef MPX_PY_SIGC_HH
#define MPX_PY_SIGC_HH


#include <sigc++/sigc++.h>
#include <boost/python.hpp>
#include <Python.h>

namespace pysigc
{
template <typename T1, typename T2, typename T3>
class sigc3
{
    private:

        sigc::signal<void, T1, T2, T3> & m_signal;
        PyObject * m_callable;

        void
        signal_conn (T1 t1, T2 t2, T3 t3)
        {
            if(m_callable)
            {
                try{
                    boost::python::call<void>(m_callable, t1, t2, t3); 
                } catch (boost::python::error_already_set)
                {
                    PyErr_Print();
                    PyErr_Clear();
                }
            }
        }

    public:

        sigc3 (sigc::signal<void, T1, T2, T3> & signal)
        : m_signal(signal)
        , m_callable(0)
        {
            m_signal.connect( sigc::mem_fun( *this, &sigc3::signal_conn ));
        }

        ~sigc3 ()
        {
        }

        void
        connect(PyObject * callable)
        {
            m_callable = callable; 
            Py_INCREF(callable);
        }

        void
        disconnect()
        {
            m_callable = 0;
        }

        void
        emit(T1 & t1, T2 & t2, T3 & t3)
        { 
            m_signal.emit(t1, t2, t3);
        }
};
}

#endif 
