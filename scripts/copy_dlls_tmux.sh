#!/bin/bash

if [[ ! -d $2/libs ]]; then mkdir $2/libs; fi
for i in $(ldd $2/collab-vm-server); do
	if [[ -f cvmlib/lib/$i ]]; then
		cp cvmlib/lib/$i $2/libs/ 2>/dev/null
	fi
done
echo '#!/bin/bash' > $2/libs/update_libs.sh
echo 'pkg install -y boost libsqlite libjpeg-turbo libpng libcairo' >> $2/libs/update_libs.sh
echo 'cd $(dirname $0)' >> $2/libs/update_libs.sh
echo 'cp *.so* $PREFIX/lib/' >> $2/libs/update_libs.sh
chmod 777 $2/libs/update_libs.sh

