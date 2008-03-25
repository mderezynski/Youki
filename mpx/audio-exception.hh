  class AudioException : public std::exception
  {
    public:
      AudioException(const std::string &msg = std::string()) : msg(msg) {}
      virtual ~AudioException() throw() {}
      const char *what() const throw() { return msg.c_str(); }
    private:
      std::string msg;
  };

#define AUDIOEXCEPTION(EXCEPTION_NAME) \
    class EXCEPTION_NAME : public AudioException \
    { \
      public: \
        EXCEPTION_NAME (std::string const& msg = std::string()) : AudioException(msg) {} \
    };
