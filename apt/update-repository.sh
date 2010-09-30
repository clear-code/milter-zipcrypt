#!/bin/sh

script_base_dir=`dirname $0`

if [ $# != 1 ]; then
    echo "Usage: $0 CODE_NAMES"
    echo " e.g.: $0 'lenny hardy lucid'"
    exit 1
fi

CODE_NAMES=$1

run()
{
    "$@"
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

update_repository()
{
    distribution=$1
    status=$2
    code_name=$3
    component=$4

    mkdir -p dists/${code_name}/${component}/binary-i386/
    mkdir -p dists/${code_name}/${component}/binary-amd64/
    mkdir -p dists/${code_name}/${component}/source/
    cat <<EOF > ../generate-${code_name}.conf
Dir::ArchiveDir ".";
Dir::CacheDir ".";
TreeDefault::Directory "pool/${code_name}/${component}";
TreeDefault::SrcDirectory "pool/${code_name}/${component}";
Default::Packages::Extensions ".deb";
Default::Packages::Compress ". gzip bzip2";
Default::Sources::Compress ". gzip bzip2";
Default::Contents::Compress "gzip bzip2";

BinDirectory "dists/${code_name}/${component}/binary-i386" {
  Packages "dists/${code_name}/${component}/binary-i386/Packages";
  Contents "dists/${code_name}/Contents-i386";
  SrcPackages "dists/${code_name}/${component}/source/Sources";
};

BinDirectory "dists/${code_name}/${component}/binary-amd64" {
  Packages "dists/${code_name}/${component}/binary-amd64/Packages";
  Contents "dists/${code_name}/Contents-amd64";
  SrcPackages "dists/${code_name}/${component}/source/Sources";
};

Tree "dists/${code_name}" {
  Sections "${component}";
  Architectures "i386 amd64 source";
};
EOF
    apt-ftparchive generate ../generate-${code_name}.conf
    rm -f dists/${code_name}/Release*
    rm -f *.db

    cat <<EOF > ../release-${code_name}.conf
APT::FTPArchive::Release::Origin "The milter manager project";
APT::FTPArchive::Release::Label "The milter manager project";
APT::FTPArchive::Release::Architectures "i386 amd64";
APT::FTPArchive::Release::Codename "${code_name}";
APT::FTPArchive::Release::Suite "${code_name}";
APT::FTPArchive::Release::Components "${component}";
APT::FTPArchive::Release::Description "milter manager packages";
EOF
    apt-ftparchive -c ../release-${code_name}.conf \
	release dists/${code_name} > /tmp/Release
    mv /tmp/Release dists/${code_name}
}

for code_name in ${CODE_NAMES}; do
    case ${code_name} in
	lenny)
	    distribution=debian
	    component=main
	    ;;
	*)
	    distribution=ubuntu
	    component=universe
	    ;;
    esac
    for status in stable development; do
	(cd ${distribution}/${status}
	    update_repository $distribution $status $code_name $component);
    done;
done
