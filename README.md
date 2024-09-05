#
#    This file is part of epics-twincat-ads.
#
#    epics-twincat-ads is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#
#    epics-twincat-ads is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
#
#    You should have received a copy of the GNU Lesser General Public License along with epics-twincat-ads. If not, see <https://www.gnu.org/licenses/>.
#
# EPICS Module to communicate with TwinCAT controllers over ADS protocol.

Originally developed for use with EEE (ESS EPICS Environment).
Can also be compiled unde for "normal" EPICS

git clone --recursive https://github.com/EuropeanSpallationSource/epics-twincat-ads

TwinCAT plc source example  can be found in TwinCAT/demo_project/

Start EPICS ioc with:
iocsh adsOnlyIO.cmd
iocsh adsMotorRecord.cmd

Start EPICS ioc with (E3):
iocsh.bash adsOnlyIO.cmd
iocsh.bash adsMotorRecord.cmd
iocsh.bash adsMotorRecordOnly.cmd

Contact: anders.sandstrom@esss.se

## Add route issue with TwinCAT version 3.1 XAE 4024
See issue: https://github.com/Beckhoff/ADS/issues/98

For TwinCAT release 4024 the ADS route needs to be added manually (dialog not working) by adding: 
```
		<Route>
			<Name>epics</Name>
			<Address>192.168.114.129</Address>
			<NetId>192.168.114.129.1.1</NetId>
			<Type>TCP_IP</Type>
			<Flags>32</Flags>
		</Route>
```
Note: Update with correct address and NetId..

to the static route file on the target cpu:
```
C:\TwinCAT\3.1\Target\StaticRoutes.xml 
```
