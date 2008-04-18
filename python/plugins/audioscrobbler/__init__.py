# $Id$
# -*- coding: utf-8 -*-
# -*- mode:python ; tab-width:4 -*- ex:set tabstop=4 shiftwidth=4 expandtab: -*-

__author__ = "Andy Theyers <andy@isotoma.com>"
__version__ = "$Revision$"[11:-2]
__docformat__ = "restructuredtext"


import datetime, locale, md5, site, sys, time, urllib, urllib2, os, mpx

try:
    # Python 2.5, module bundled:
    from xml.etree import ElementTree
except:
    # Python 2.4, separate module:
    from elementtree import ElementTree

# This is lifted in the most part from iPodScrobbler (see docs above)
# Get the base local encoding
enc = 'ascii'
try:
    enc = site.encoding
except:
    pass
if enc == 'ascii' and locale.getpreferredencoding():
    enc = locale.getpreferredencoding()
# if we are on MacOSX, we default to UTF8
# because apples python reports 'ISO8859-1' as locale, but MacOSX uses utf8
if sys.platform=='darwin':
    enc = 'utf8'

# AudioScrobblerQuery configuration settings
audioscrobbler_request_version = '1.0'
audioscrobbler_request_host = 'ws.audioscrobbler.com'

# AudioScrobblerPost configuration settings
audioscrobbler_post_version = u'1.2' #updated
audioscrobbler_post_host = 'post.audioscrobbler.com'
client_name = u'mpx'
pyscrobbler_version = u'1.0.0.0' # This is set to 1.0 while we use
                             # client_name = u'tst' as we keep getting
                             # UPDATE responses with anything less.

class AudioScrobblerError(Exception):
    
    """
    The base AudioScrobbler error from which all our other exceptions derive
    """
    
    def __init__(self, message):
        """ Create a new AudioScrobblerError.

            :Parameters:
                - `message`: The message that will be displayed to the user
        """
        self.message = message
    
    def __repr__(self):
        msg = "%s: %s"
        return msg % (self.__class__.__name__, self.message,)
    
    def __str__(self):
        return self.__repr__()

class AudioScrobblerConnectionError(AudioScrobblerError):
    
    """
    The base network error, raise by invalid HTTP responses or failures to
    connect to the server specified (DNS, timeouts, etc.)
    """
    
    def __init__(self, type, code, message):
        self.type = type
        self.code = code
        self.message = message
    
    def __repr__(self):
        msg = "AudioScrobblerConnectionError: %s: %s %s"
        return msg % (self.type.upper(), self.code, self.message,)
    
class AudioScrobblerTypeError(AudioScrobblerError):
    
    """
    If we would normally raise a TypeError we raise this instead
    """
    pass
    

class AudioScrobblerPostUpdate(AudioScrobblerError):
    
    """
    If the POST server returns an ``UPDATE`` message we raise this exception.
    This is the only case when this exception is raised, allowing you to
    happily ignore it.
    """
    
    def __repr__(self):
        msg = "An update to your AudioScrobbler client is available at %s"
        return msg % (self.message,)


class AudioScrobblerPostFailed(AudioScrobblerError):
    
    """
    If the POST server returns an ``FAILED`` message we raise this exception.
    """
    
    def __repr__(self):
        msg = "Posting track to AudioScrobbler failed.  Reason given: %s"
        return msg % (self.message,)


class AudioScrobblerHandshakeError(AudioScrobblerError):
    
    """
    If we fail the handshake this is raised.  If you're running in a long
    running process you should pass on this error, as the system keeps a
    cache which ``flushcache()`` will clear for you when the server is back.
    """
    pass

class AudioScrobbler:
    
    """ Factory for Queries and Posts.  Holds configuration for the session """
    
    def __init__(self,
                 audioscrobbler_request_version=audioscrobbler_request_version,
                 audioscrobbler_post_version=audioscrobbler_post_version,
                 audioscrobbler_request_host=audioscrobbler_request_host,
                 audioscrobbler_post_host=audioscrobbler_post_host,
                 client_name=client_name,
                 client_version=pyscrobbler_version):
                 
        self.audioscrobbler_request_version=audioscrobbler_request_version
        self.audioscrobbler_post_version=audioscrobbler_post_version
        self.audioscrobbler_request_host=audioscrobbler_request_host
        self.audioscrobbler_post_host=audioscrobbler_post_host
        self.client_name=client_name
        self.client_version=pyscrobbler_version
        
    def query(self, **kwargs):
        
        """ Create a new AudioScrobblerQuery """
        
        if len(kwargs) != 1:
            raise TypeError("__init__() takes exactly 1 argument, %s "
                            "given" % (len(kwargs),))
        ret = AudioScrobblerQuery(version=self.audioscrobbler_request_version,
                                  host=self.audioscrobbler_request_host,
                                  **kwargs)
        return ret
        
    def post(self, username, password, verbose=False):
        
        """ Create a new AudioScrobblerPost """
        
        ret = AudioScrobblerPost(username=username.encode('utf8'),
                                 password=password.encode('utf8'),
                                 host=self.audioscrobbler_post_host,
                                 protocol_version=self.audioscrobbler_post_version,
                                 client_name=self.client_name,
                                 client_version=self.client_version,
                                 verbose=verbose)
        return ret
        

class AudioScrobblerCache:
    def __init__(self, elemtree, last):
        self.elemtree = elemtree
        self.requestdate = last
        
    def created(self):
        return self.requestdate
        
    def gettree(self):
        return self.elemtree

        
class AudioScrobblerQuery:
    
    def __init__(self,
                 version=audioscrobbler_request_version, 
                 host=audioscrobbler_request_host, 
                 **kwargs):
        
        if len(kwargs) != 1:
            raise TypeError("__init__() takes exactly 1 audioscrobbler "
                            "request argument, %s given" % str(len(kwargs))
                           )
        self.type = kwargs.keys()[0]
        self.param = str(kwargs[self.type])
        self.baseurl = 'http://%s/%s/%s/%s' % (host, 
                                               version, 
                                               urllib.quote(self.type), 
                                               urllib.quote(self.param), 
                                              )
        self._cache = {}
        
    def __getattr__(self, name):
        def method(_self=self, name=name, **params):
            # Build the URL
            url = '%s/%s.xml' % (_self.baseurl, urllib.quote(name))
            if len(params) != 0:
                for key in params.keys():
                    # This little mess is required to get round the fact that
                    # 'from' is a reserved word and can't be passed as a
                    # parameter name.
                    if key.startswith('_'):
                        params[key[1:]] = params[key]
                        del params[key]
                url = '%s?%s' % (url, urllib.urlencode(params))
            req = urllib2.Request(url)
            
            # Check the cache
            cache = _self._cache
            if url in cache and cache[url].created() is not None:
                req.add_header('If-Modified-Since', cache[url].created())
            
            # Open the URL and test the response
            try:
                response = urllib2.urlopen(url)
            except urllib2.HTTPError, error:
                if error.code == 304:
                    return AudioScrobblerItem(cache[url].getroot(), _self, url)
                if error.code == 400:
                    raise AudioScrobblerConnectionError('ws', 400, error.fp.read())
                raise AudioScrobblerConnectionError('http', error.code, error.msg)
            except urllib2.URLError, error:
                code = error.reason.args[0]
                message = error.reason.args[1]
                raise AudioScrobblerConnectionError('network', code, message)
            elemtree = ElementTree.ElementTree(file=response)
            if response.headers.get('pragma', None) != 'no-cache':
                last_modified = response.headers.get('last-modified', None)
            else:
                last_modified = None
            _self._cache[url] = AudioScrobblerCache(elemtree, last_modified)
            return AudioScrobblerItem(elemtree.getroot(), _self, url)
        return method
        
    def __repr__(self):
        return "AudioScrobblerQuery(%s='%s')" % (self.type, self.param)
        
    def __str__(self):
        return self.__repr__()

        
class AudioScrobblerItem:
    
    def __init__(self, element, parent, url=None):
        self._element = element
        self._parent = parent
        self.tag = element.tag
        self.text = element.text
        self._url = url
        if self._url is None:
            self._url = self._parent._url
    
    def __repr__(self):
        if isinstance(self._parent, AudioScrobblerQuery):
            return "<AudioScrobbler response from %s>" % (self._url,)
        return "<AudioScrobbler %s element at %s>" % (self.tag, id(self))
        
    def __str__(self):
        if self.text is None:
            return ''
        text = self.text
        try:
            retval = text.encode(enc)
        except AttributeError:
            retval = text
        return retval
    
    def __getattr__(self, name):
        result = self._element.findall(name)
        if len(result) == 0:
            raise AttributeError("AudioScrobbler %s element has no "
                                 "subelement '%s'" % (self.tag, name)
                                )
        ret = [ AudioScrobblerItem(i, self) for i in result ]
        if len(ret) == 1:
            return ret[0]
        return ret
    
    def __iter__(self):
        for i in self._element:
            yield AudioScrobblerItem(i, self)
            
    def __getitem__(self, i):
        return self._element.attrib[i]
        
    def get(self, i, default):
        if i in self._element.attrib:
            return self._element.attrib[i]
        else:
            return default
        
    def __getslice__(self, i, j):
        return [ AudioScrobblerItem(x, self) for x in self._element[i:j] ]
    
    def raw(self):
        def getparent(obj):
            if isinstance(obj._parent, AudioScrobblerQuery):
                return obj._parent
            return getparent(obj._parent)
        return getparent(self)._cache[self._url].gettree()
    
    def element(self):
        return self._element


class AudioScrobblerPost:
    
    """
    Provide the ability to post tracks played to a user's Last.fm
    account
    """
    #Updated to protocol version 1.2
    
    def __init__(self,
                 username=u'',
                 password=u'',
                 client_name=client_name,
                 client_version=pyscrobbler_version,
                 protocol_version=audioscrobbler_post_version,
                 host=audioscrobbler_post_host,
                 verbose=False):
    
        # Capture the information passed for future use
        self.params = dict(username=username,
                           password=password,
                           client_name=client_name,
                           client_version=client_version,
                           protocol_version=protocol_version,
                           host=host)
        
        self.verbose = verbose
        
        self.auth_details = {}
        self.last_post = None
        self.interval = 0
        self.cache = []
        self.loglines = []
        self.last_shake = None
        self.authenticated = False
        self.updateurl = None
        self.posturl = None
        self.npurl = None
        
    def auth(self):
        
        """ Authenticate against the server """
        
        if self.authenticated:
            return True
        
        timestamp = str(int(time.time()))
        password = self.params['password']
        auth_token = md5.md5(md5.md5(password).hexdigest() + timestamp).hexdigest()
        
        p = {}
        p['hs'] = 'true'
        p['p'] = self.params['protocol_version']
        p['c'] = self.params['client_name']
        p['v'] = self.params['client_version']
        p['u'] = self.params['username']
        p['t'] = timestamp
        p['a'] = auth_token
        
        plist = [(k, urllib.quote_plus(v.encode('utf8'))) for k, v in p.items()]
        
        authparams = urllib.urlencode(plist)
        url = 'http://%s/?%s' % (self.params['host'], authparams)
        req = urllib2.Request(url)
        try:
            url_handle = urllib2.urlopen(req)
        except urllib2.HTTPError, error:
            self.authenticated = False
            raise AudioScrobblerConnectionError('http', error.code, error.msg)
        except urllib2.URLError, error:
            self.authenticated = False
            code = error.reason#.args[0]
            message = error.reason#.args[1]
            raise AudioScrobblerConnectionError('network', code, message)
        
        response = url_handle.readlines()
        if len(response) == 0:
            raise AudioScrobblerHandshakeError('Got nothing back from the server')  
            
        username = self.params['username']
        password = self.params['password']
        
        # First we test the best and most likely case
        if response[0].startswith('OK'):
            answer = response[1].strip()
            self.auth_details['s'] = answer
            self.npurl = response[2].strip()
            self.posturl = response[3].strip()
            self.authenticated = True           
            
        # Then the various failure states....
        elif response[0].startswith('BANNED'):
            self.authenticated = False
            msg = "this client version has been banned from the server."
            raise AudioScrobblerHandshakeError(msg)
        
        elif response[0].startswith('BADAUTH'):
            self.authenticated = False
            msg = "The authentication details provided were incorrect."
            raise AudioScrobblerHandshakeError(msg)
        
        elif response[0].startswith('BADTIME'):
            self.authenticated = False
            msg = "The timestamp provided was not close enough to the current time."
            raise AudioScrobblerHandshakeError(msg) 
        
        elif response[0].startswith('FAILED'):
            self.authenticated = False
            reason = response[0][6:]
            try:
                msg = reason.decode('utf8').encode(enc)
            except:
                msg = reason
            raise AudioScrobblerHandshakeError(msg)
            
        else:
            self.authenticated = False
            msg = "Unknown response from server: %r" % (response,)
            raise AudioScrobblerHandshakeError(msg)
            
    def posttrack(self, 
                  artist_name,
                  song_title,
                  length,
                  date_played, 
                  tracknumber=u'',
                  album=u'',
                  mbid=u'',
                  source=u'P'):
        
        """
        Add the track to the local cache, and try to post it to the server
        """
        
        self.addtrack(artist_name=artist_name,
                      song_title=song_title,
                      length=length,
                      date_played=date_played,
                      tracknumber=tracknumber,
                      album=album,
                      mbid=mbid,
                      source=source)
        self.post()
        
    __call__ = posttrack
    
    def addtrack(self, 
                 artist_name,
                 song_title,
                 length,
                 date_played, 
                 tracknumber=u'',
                 album=u'',
                 mbid=u'',
                 source=u'P'):
        
        """ Add a track to the local cache """
        
        # Quick sanity check on track length
        if type(length) == type(0):
            sane_length = length
        elif type(length) == type('') and length.isdigit():
            sane_length = int(length)
        else:
            sane_length = -1
        
        # If the track length is less than 30 move on
        if sane_length < 30:
            msg = ("Track '%s' by '%s' only %s seconds in length, so "
                   "not added" % (song_title, artist_name, sane_length))
            self.log(msg)
        # Otherwise build the track dictionary and add it to the local cache
        else:
            try:
                track = {'a[%s]': artist_name.decode(enc).encode('utf8'),
                        't[%s]': song_title.decode(enc).encode('utf8'),
                        'l[%s]': str(sane_length),
                        'i[%s]': date_played,
                        'b[%s]': album.decode(enc).encode('utf8'),
                        'm[%s]': mbid.encode('utf8'),
                        'r[%s]': u'',
                        'n[%s]': tracknumber.decode(enc).encode('utf8'),
                        'o[%s]': source.decode(enc).encode('utf8'),
                        }
            except:
                track = {'a[%s]': artist_name,
                        't[%s]': song_title,
                        'l[%s]': str(sane_length),
                        'i[%s]': date_played,
                        'b[%s]': album,
                        'm[%s]': mbid.encode('utf8'),
                        'r[%s]': u'',
                        'n[%s]': tracknumber.encode('utf8'),
                        'o[%s]': source.encode('utf8'),
                        }
            self.cache.append(track)
    
    def nowplaying(self, 
                  artist_name,
                  song_title,
                  length=u'',
                  tracknumber=u'', 
                  album=u'',
                  mbid=u''):

        self.auth()
        
        p = {}
        p['s'] = self.auth_details['s']
        
        try:
            p['a'] = artist_name.decode(enc).encode('utf8')
            p['t'] = song_title.decode(enc).encode('utf8')
            p['b'] = album.decode(enc).encode('utf8')
            p['l'] = length.encode('utf8')
            p['n'] = tracknumber.encode('utf8')
            p['m'] = mbid.encode('utf8')
        except:
            p['a'] = artist_name
            p['t'] = song_title
            p['b'] = album
            p['l'] = length.encode('utf8')
            p['n'] = tracknumber.encode('utf8')
            p['m'] = mbid.encode('utf8')
                        
        npdata = urllib.urlencode(p)
        
        req = urllib2.Request(url=self.npurl, data=npdata)
        
        try:
            url_handle = urllib2.urlopen(req)
        except urllib2.HTTPError, error:
            self.authenticated = False
            print AudioScrobblerConnectionError('http', error.code, error.msg)
            return
        except:
            code = '000'
            message = sys.exc_info()[1]
            print AudioScrobblerConnectionError('network', code, message)
            return
                
        response = url_handle.readlines()
        if response[0].startswith('OK'):
            self.log("Now playing track updated ('%s' by '%s')." % (p['t'], p['a']))
        elif response[0].startswith('BADSESSION'):
            self.authenticated = False
        
    def post(self):
        
        """
        Post the tracks by popping the first ten off the cache and attempting
        to post them.
        """
        
        if len(self.cache) == 0:
            return
        if len(self.cache) > 10:
            number = 10
        else:
            number = len(self.cache)
        
        params = {}
        count = 0
        for track in self.cache[:number]:
            for k in track.keys():
                params[k % (count,)] = track[k]
            count += 1
        
        self.auth()
        params.update(self.auth_details)
        postdata = urllib.urlencode(params)
        req = urllib2.Request(url=self.posturl, data=postdata)
        
        now = datetime.datetime.utcnow()
        
        try:
            url_handle = urllib2.urlopen(req)
        except urllib2.HTTPError, error:
            raise AudioScrobblerConnectionError('http', error.code, error.msg)
        except urllib2.URLError, error:
            args = getattr(error.reason, 'args', None)
            code = '000'
            message = str(error)
            if args is not None:
                if len(args) == 1:
                    message = error.reason.args[0]
                elif len(args) == 2:
                    code = error.reason.args[0]
                    message = error.reason.args[1]
            raise AudioScrobblerConnectionError('network', code, message)
        except:
            code = '000'
            message = sys.exc_info()[1]
            raise AudioScrobblerConnectionError('network', code, message)
        
        self.last_post = now
        response = url_handle.readlines()
            
        # Test the various responses possibilities:
        if response[0].startswith('OK'):
            for song in self.cache[:number]:
                self.log("Uploaded track successfully ('%s' by '%s')." % (song['t[%s]'], song['a[%s]']))
            del self.cache[:number]
        elif response[0].startswith('BADSESSION'):
            self.log("Got BADSESSION.")
            self.authenticated = False
        elif response[0].startswith('FAILED'):
            reason = response[0][6:].strip()
            raise AudioScrobblerPostFailed(reason)
        else:
            msg = "Server returned something unexpected."
            raise AudioScrobblerPostFailed(msg)
        
    def flushcache(self):
        
        """ Post all the tracks in the cache """
        
        while len(self.cache) > 0:
            self.post()
    
    def savecache(self, filename):
        
        """ Save all tracks in the cache to a file """

        #conf = ConfigParser.ConfigParser()
        
        # Save each track in cache:
        #count = 0
        #for track in self.cache:
        #   conf.add_section('Track ' + str(count))
        #   for k in track.keys():
        #       conf.set('Track ' + str(count), k, track[k])
        #   count += 1
            
        #conf.write(file(filename, 'w'))
        
    def retrievecache(self, filename):
        
        """ 
        Retrieve all cached tracks from a file so that cached tracks
        are preserved across client restarts
        """

        #if not os.path.isfile(filename):
        #   return
        
        #conf = ConfigParser.ConfigParser()
        #conf.read(filename)

        # Retrieve each cached track from file:
        #count = 0
        #while conf.has_section('Track ' + str(count)):
        #   track = {}
        #   for key in ['a','t','l','i','b','m','r','n','o']:
        #       if conf.has_option('Track ' + str(count), key + '[%s]'):
        #           track[key + '[%s]'] = conf.get('Track ' + str(count), key + '[%s]')
        #   self.cache.append(track)
        #   count += 1
        
        # Cached tracks retrieved, delete file:
        #os.remove(filename)
        
    def log(self, msg):
        
        """ Add a line to the log, print it if verbose """
        
        time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        self.loglines.append("%s: %s" % (time, msg))
        if self.verbose:
            print self.loglines[-1]
            
    def getlog(self, clear=False):
        
        """ Return the entire log, clear it if requested """
        
        if clear:
            retval = self.loglines
            self.loglines = []
            return retval
        return self.loglines

class MPXAudioScrobbler(mpx.Plugin):

    def activate(self,player,mcs):
    
        """ Activate plugin """

        self.player = player
        self.as = AudioScrobbler()
        self.post = self.as.post(mcs.key_get_string("lastfm", "username"), mcs.key_get_string("lastfm", "password"))
        self.post.auth()
        self.hand1 = self.player.gobj().connect("track-played", self.track_played)
        self.hand2 = self.player.gobj().connect("new-track", self.now_playing)
        self.player_playstatus_changed = self.player.gobj().connect("play-status-changed", self.pstate_changed)

        return True

    def deactivate(self):

        """ Deactivate plugin """

        self.player.gobj().disconnect(self.hand1)
        self.player.gobj().disconnect(self.hand2)
        self.player = None
        self.as = None
        self.post = None

    def pstate_changed(self, blah, state):

        if state == mpx.PlayStatus.STOPPED:
            self.post.flushcache ()
        
    def track_played(self, blah):

        p_date = time.time()
        m = self.player.get_metadata()

        try:
            p_artist = m.get(mpx.AttributeId.ARTIST).val().get_string()
            p_title = m.get(mpx.AttributeId.TITLE).val().get_string()
        except:
            p_artist = None
            p_title = None

        if p_artist != None and p_title != None: 

            p_artist = m.get(mpx.AttributeId.ARTIST).val().get_string()
            p_title = m.get(mpx.AttributeId.TITLE).val().get_string()

            try:
                m_len = m.get(mpx.AttributeId.TIME)
                p_len = m_len.val().get_int()
            except:
                p_len = 0

            try:
                m_album = m.get(mpx.AttributeId.ALBUM)
                p_album = m_album.val().get_string()
            except:
                p_album=u''

            try:
                m_tracknumber = m.get(mpx.AttributeId.TRACK)
                p_tracknumber = str(m_tracknumber.val().get_int())
            except:
                p_tracknumber=u''

            try:
                m_mbid = m.get(mpx.AttributeId.MB_TRACK_ID)
                p_mbid = m_mbid.val().get_string()
            except:
                p_mbid=u''

            print "Posting track with MBID: " + p_mbid + " at date " + str(p_date)

            self.post.posttrack(str(p_artist),
                                str(p_title),
                                str(p_len),
                                str(int(p_date)),
                                str(p_tracknumber),
                                str(p_album),
                                str(p_mbid))

    def now_playing(self, blah):

        m = self.player.get_metadata()

        try:
            p_artist = m.get(mpx.AttributeId.ARTIST).val().get_string()
            p_title = m.get(mpx.AttributeId.TITLE).val().get_string()
        except:
            p_artist = None
            p_title = None

        if p_artist != None and p_title != None: 

            p_artist = m.get(mpx.AttributeId.ARTIST).val().get_string()
            p_title = m.get(mpx.AttributeId.TITLE).val().get_string()

            try:
                m_len = m.get(mpx.AttributeId.TIME)
                p_len = m_len.val().get_int()
            except:
                p_len = 0

            try:
                m_album = m.get(mpx.AttributeId.ALBUM)
                p_album = m_album.val().get_string()
            except:
                p_album=u''

            try:
                m_tracknumber = m.get(mpx.AttributeId.TRACK)
                p_tracknumber = str(m_tracknumber.val().get_int())
            except:
                p_tracknumber=u''

            try:
                m_mbid = m.get(mpx.AttributeId.MB_TRACK_ID)
                p_mbid = m_mbid.val().get_string()
            except:
                p_mbid=u''

            print "Posting now-playing with MBID: " + p_mbid + " at date " + str(time.time())

            self.post.flushcache ()
            self.post.nowplaying(str(p_artist),
                                 str(p_title),
                                 str(p_len),
                                 str(p_tracknumber),
                                 str(p_album),
                                 str(p_mbid))
