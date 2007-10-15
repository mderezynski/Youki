dnl -*- Mode: Autoconf; -*-

dnl BMP_CHECK_TR1
AC_DEFUN([BMP_CHECK_TR1],
           [BMP_WRAPPED_CHECK([AC_MSG_CHECKING([for C++ tr1 extensions])
                                   AC_TRY_RUN([
                                    #include <tr1/unordered_map>
                                    #include <tr1/unordered_set>
                                    #include <string>
                                    typedef std::tr1::unordered_map<std::string, long> MapType;
                                    typedef std::tr1::unordered_set<std::string> SetType;
                                    typedef MapType::value_type ValuePair;
                                    typedef SetType::value_type Value;
                                    int main (int n_args, char **args)
                                    {
                                      MapType m;  
                                      SetType s;
                                      return 0; 
                                    }],
                                    have_tr1=yes,
                                    have_tr1=no)])])
