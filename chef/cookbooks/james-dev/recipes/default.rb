package "alsa"
package "libasound2-dev"
package "libflac-dev"
package "build-essential"
package "python2.7-dev"
package "python-pip"
package "git-core"
package "mercurial"
package "libzmq-dev"
package "libevent-dev"
package "gdb"
package "htop"
package "jackd"
package "libjack-dev"
package "jack-tools"

file "/etc/jackdrc" do
  content "/usr/bin/jackd  -Z -d alsa -d hw:miniStreamer -C -n 24"
end


python_pip "pyzmq" do
  action :install
end

python_pip "tornado" do
  action :install
end

python_pip "git+https://github.com/MrJoes/tornadio2.git" do
  action :install
end

git "/home/ubuntu/src" do
  repository "https://github.com/drue/King-James.git"
  action :sync
end

### building images
package "uboot-mkimage"
package "btrfs-tools"
package "pv"

