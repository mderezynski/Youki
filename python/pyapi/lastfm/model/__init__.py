
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
