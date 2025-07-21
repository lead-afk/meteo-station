#include <Preferences.h>
#include <vector>

using namespace std;

Preferences prefs;

template <typename T>
void saveVector(const string &key, const vector<T> &vec)
{
    prefs.begin("storage", false);

    size_t byteSize = vec.size() * sizeof(T);
    prefs.putBytes(key.c_str(), vec.data(), byteSize);
    prefs.putUInt((key + "_size").c_str(), vec.size());

    prefs.end();
}

template <typename T>
vector<T> loadVector(const string &key)
{
    prefs.begin("storage", true);

    uint32_t size = prefs.getUInt((key + "_size").c_str(), 0);
    vector<T> vec;

    if (size > 0)
    {
        vec.resize(size);
        prefs.getBytes(key.c_str(), vec.data(), size * sizeof(T));
    }

    prefs.end();
    return vec;
}
