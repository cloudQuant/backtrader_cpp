#!/usr/bin/env python
# -*- coding: utf-8; py-indent-offset:4 -*-
###############################################################################
#
# Copyright (C) 2015-2023 Daniel Rodriguez
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

from . import testcommon

import backtrader_cpp as bt
btind = bt.indicators

chkdatas = 1
chkvals = [
    ['4063.463000', '3644.444667', '3554.693333'],
    ['4165.049575', '3735.555783', '3643.560667'],
    ['3961.876425', '3553.333550', '3465.826000'],
]

chkmin = 30
chkind = btind.SMAEnvelope


def test_run(main=False):
    datas = [testcommon.getdata(i) for i in range(chkdatas)]
    
    # Debug: print data length and envelope info
    import backtrader_cpp as bt
    
    class DebugStrategy(testcommon.TestStrategy):
        def __init__(self):
            super().__init__()
            smaenv = btind.SMAEnvelope(self.data.close)
            print(f"DEBUG: Data length = {len(self.data)}")
            print(f"DEBUG: SMA line buffer length = {smaenv.lines.sma.buflen()}")
            print(f"DEBUG: Top line buffer length = {smaenv.lines.top.buflen()}")
            print(f"DEBUG: Bot line buffer length = {smaenv.lines.bot.buflen()}")
    
    testcommon.runtest(datas,
                       DebugStrategy if main else testcommon.TestStrategy,
                       main=main,
                       plot=main,
                       chkind=chkind,
                       chkmin=chkmin,
                       chkvals=chkvals)


if __name__ == '__main__':
    test_run(main=True)
