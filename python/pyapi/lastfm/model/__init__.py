class TrackTopTag():

    def __init__(self):

        self.name  = None 
        self.count = None
        self.url   = None
    
    def getName(self):

        return self.name

    def setName(self, name):

        self.name =name

    def getCount(self):

        return self.count

    def setCount(self, count):

        try:
            self.count = int(count)
        except:
            self.count = None 

    def getUrl(self):

        return self.url

    def setUrl(self, url):

        self.url = url

class SimilarTrackArtist():

    def __init__(self):

        self.name = None
        self.url  = None

    def getName(self):

        return self.name

    def setName(self, name):

        self.name = name

    def getUrl(self):

        return self.url
    
    def setUrl(self, url):

        self.url = url

class SimilarTrack():

    def __init__(self):

        self.name  = None 
        self.match = None
        self.url   = None
        self.streamable = None
        self.artist = None

    def getArtist(self):

        return self.artist

    def setArtist(self, artist):

        self.artist = artist

    def getName(self):

        return self.name

    def setName(self, name):

        self.name = name

    def getMatch(self):

        return self.match

    def setMatch(self, match):

        try:
            self.match = float(match)
        except:
            self.match = None 

    def getUrl(self):

        return self.url

    def setUrl(self, url):

        self.url = url

    def getStreamable(self):

        return self.streamable

    def setStreamable(self, streamable):

        self.streamable = int(streamable)
