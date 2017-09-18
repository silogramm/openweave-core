#!/usr/bin/env python


#
#    Copyright (c) 2016-2017 Nest Labs, Inc.
#    All rights reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

#
#    @file
#       Calls Weave Alarm test between nodes.
#

import itertools
import os
import unittest
import set_test_path

from happy.Utils import *
import happy.HappyNodeList
import plugins.weave.WeaveStateLoad as WeaveStateLoad
import plugins.weave.WeaveStateUnload as WeaveStateUnload
import plugins.weave.WeaveAlarm as WeaveAlarm
import plugin.WeaveUtilities as WeaveUtilities


class test_weave_alarm_01(unittest.TestCase):
    def setUp(self):
        self.tap = None

        if "WEAVE_SYSTEM_CONFIG_USE_LWIP" in os.environ.keys() and os.environ["WEAVE_SYSTEM_CONFIG_USE_LWIP"] == "1":
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../topologies/three_nodes_on_tap_thread_weave.json"
            self.tap = "wpan0"
        else:
            self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
                "/../topologies/three_nodes_on_thread_weave.json"

        self.show_strace = False

        # setting Mesh for thread test
        options = WeaveStateLoad.option()
        options["quiet"] = True
        options["json_file"] = self.topology_file

        setup_network = WeaveStateLoad.WeaveStateLoad(options)
        ret = setup_network.run()

    def tearDown(self):
        # cleaning up
        options = WeaveStateUnload.option()
        options["quiet"] = True
        options["json_file"] = self.topology_file

        teardown_network = WeaveStateUnload.WeaveStateUnload(options)
        teardown_network.run()


    def test_weave_alarm(self):
        options = happy.HappyNodeList.option()
        options["quiet"] = True

        value, data = self.__run_alarm_test_between("node01", "node02")
        self.__process_result("node01", "node02", value)

    def __process_result(self, nodeA, nodeB, value):
        print "alarm test among " + nodeA + " and " + nodeB

        if value:
            print hgreen("Passed")
        else:
            print hred("Failed")
            raise ValueError("Weave Alarm Test Failed")


    def __run_alarm_test_between(self, nodeA, nodeB):
        options = WeaveAlarm.option()
        options["quiet"] = False
        options["originator"] = nodeA
        options["remote"] = nodeB
        options["tap"] = self.tap

        weave_alarm = WeaveAlarm.WeaveAlarm(options)
        ret = weave_alarm.run()

        value = ret.Value()
        data = ret.Data()

        return value, data


if __name__ == "__main__":
    WeaveUtilities.run_unittest()


