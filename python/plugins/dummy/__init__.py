#
# MPX Dummy plugin
# (C) 2008 D. Le Brun
#

import mpx
#import mpx_playlist

class Dummy(mpx.Plugin):

    def activate(self,player,mcs):
        print ">> Dummy Plugin activated"
        self.player = player
        return True

    def deactivate(self):
        print ">> Dummy Plugin deactivated"
        self.player = None

    def run(self):
        print ">> Dummy Plugin running"
