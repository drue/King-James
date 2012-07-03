package "alsa"
package "libasound2-dev"
package "libflac-dev"
package "libflac++-dev"
package "build-essential"
package "python2.7-dev"
package "python-pip"
package "git-core"
package "mercurial"
package "libzmq-dev"
package "libevent-dev"
package "gdb"
package "htop"
package "libjack-dev"
package "libboost-dev"
package "libboost-thread-dev"
package "avahi-daemon"
package "unzip"

python_pip "pyzmq" do
  action :install
end

python_pip "tornado" do
  action :install
end

python_pip "git+https://github.com/mrjoes/tornadio2.git" do
  action :install
end
