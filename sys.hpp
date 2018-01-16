#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <sys/stat.h>


struct sysinfo {
    private:
        struct utsname un;
        struct statvfs dsk;
        void rplc(std::string *s, const std::string &obj, const std::string &subs) {
            for(int i = 0; ; i += subs.length()) {
                i = s->find(obj, i);
                if(i == std::string::npos) break;
                s->erase(i, obj.length());
                s->insert(i, subs);
            }
        }
        void trim(std::string *str) {
            while (str->find("  ") != std::string::npos) {
                rplc(str, "  ", " ");
            }
        }
        void mfreq(std::string *cpustr) {
            std::ifstream mfreqfile("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
            std::string content((std::istreambuf_iterator<char>(mfreqfile)), std::istreambuf_iterator<char>());
            if (mfreqfile.good()) {
                std::string output = cpustr->substr(0, cpustr->find_last_of("at")+2) + (char)content[0] + (std::string)"." + (char)content[1] + " GHz";
                *cpustr = output;
            }
        }
        std::string npackages(bool cache) {   // Please, do not laugh at me
            struct stat buffer;
            if (!stat("/tmp/farfetch_npackages", &buffer) == 0 || !cache) {
                std::system("bash -c 'packages=0;\
                type -p pacman >/dev/null && packages=\"$((packages+=$(pacman -Qq --color never | wc -l)))\";\
                type -p dpkg >/dev/null && packages=\"$((packages+=$(dpkg --get-selections | grep -cv deinstall$)))\";\
                type -p kpm >/dev/null && packages=\"$((packages+=$(kpm --get-selections | grep -cv deinstall$)))\";\
                type -p pkgtool >/dev/null && packages=\"$((packages+=$(ls -1 /var/log/packages | wc -l)))\";\
                type -p rpm >/dev/null && packages=\"$((packages+=$(rpm -qa | wc -l)))\";\
                type -p pkginfo >/dev/null && packages=\"$((packages+=$(pkginfo -i | wc -l)))\";\
                type -p emerge >/dev/null && packages=\"$((packages+=$(ls -d /var/db/pkg/*/* | wc -l)))\"; echo $packages > /tmp/farfetch_npackages'");
            }
            std::ifstream nr("/tmp/farfetch_npackages");
            std::string line;
            getline(nr, line);
            return line;
        }
        std::string _getAttribFromRaw(const char* file, std::string attrib, char sep) {
            std::ifstream raw(file);
            std::string line;
            while (std::getline(raw,line)) {
                if (line.substr(0,attrib.length()) == attrib){
                    return line.substr(line.find(sep)+2);
                }
            }
        }

    public:
        int disk_used;

        std::map<const std::string, std::string> modules = {
            {"Kernel",      "err"},
            {"Host",        "err"},
            {"CPU",         "err"},
            {"Packages",    "err"}
        };

        std::map<std::string, int> bars = {
            {"disk",        0}
        };

        sysinfo(ini *config) {
            std::string cpu_module = _getAttribFromRaw("/proc/cpuinfo", "model name", ':');
            rplc(&cpu_module, "(R)", "");
            rplc(&cpu_module, "(r)", "");
            rplc(&cpu_module, "(TM)", "");
            rplc(&cpu_module, "(tm)", "");
            rplc(&cpu_module, "Core", "");
            rplc(&cpu_module, "CPU", "");
            rplc(&cpu_module, "@", "at");
            mfreq(&cpu_module);
            trim(&cpu_module);

            uname(&un);
            this->modules["Kernel"] = this->un.release;
            this->modules["Host"] = this->un.nodename;
            this->modules["CPU"] = cpu_module;
            this->modules["Packages"] = npackages(config->modules["pkgcache"]);

            statvfs(config->bars.disk.c_str(), &dsk);
            this->disk_used, bars["disk"] = 100 - (unsigned long)this->dsk.f_bavail * 100 / (unsigned long)this->dsk.f_blocks;
        }
};