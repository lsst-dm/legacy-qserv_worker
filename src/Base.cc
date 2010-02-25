// Basic convention/API-related things that might be shared.

// Std
#ifdef __SUNPRO_CC
#include <sys/md5.h>
#else // Linux?
#include <openssl/md5.h>
#endif

// Boost
#include <boost/format.hpp>

// Myself:
#include "lsst/qserv/worker/Base.h"

namespace qWorker = lsst::qserv::worker;

// Local helpers
namespace {
template <class T> struct ptrDestroy {
    void operator() (T& x) { delete[] x.buffer;}
};

template <class T> struct offsetLess {
    bool operator() (T const& x, T const& y) { return x.offset < y.offset;}
};
}


//////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////
// Must end in a slash.
std::string qWorker::DUMP_BASE = "/tmp/qserv/";

std::string qWorker::CREATE_SUBCHUNK_SCRIPT =
    "CREATE DATABASE IF NOT EXISTS Subchunks_%1%;"
    "CREATE TABLE IF NOT EXISTS Subchunks_%1%.Object_%1%_%2% ENGINE = MEMORY "
    "AS SELECT * FROM LSST.Object_%1% WHERE subchunkId = %2%;";
std::string qWorker::CLEANUP_SUBCHUNK_SCRIPT =
    "DROP TABLE Subchunks_%1%.Object_%1%_%2%;";


//////////////////////////////////////////////////////////////////////
// Hashing-related
//////////////////////////////////////////////////////////////////////
#ifdef __SUNPRO_CC // MD5(...) not defined on Solaris's ssl impl.
namespace {
    inline unsigned char* MD5(unsigned char const* d,
			      unsigned long n,
			      unsigned char* md) {
	// Defined with RFC 1321 MD5 functions.
	MD5_CTX ctx;
	assert(md != NULL); // Don't support null input.
	MD5Init(&ctx);
	MD5Update(&ctx, d, n);
	MD5Final(md, &ctx);
	return md;
    }
}
#endif

std::string qWorker::hashQuery(char const* buffer, int bufferSize) {
    unsigned char hashVal[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<unsigned char const*>(buffer), bufferSize, hashVal);
#ifdef DO_NOT_USE_BOOST
    return qWorker::hashFormat(hashVal, MD5_DIGEST_LENGTH);
#else
    std::string result;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        result += (boost::format("%02x") % static_cast<int>(hashVal[i])).str();
    }
    return result;
#endif
}

std::string qWorker::hashToPath(std::string const& hash) {
    return DUMP_BASE +
        hash.substr(0, 3) + "/" + hash.substr(3, 3) + "/" + hash + ".dump";
}

std::string qWorker::hashToResultPath(std::string const& hash) {
    // Not sure whether we want a different path later.
    // For now, drop the .dump extension.
    //    return DUMP_BASE +
    //        hash.substr(0, 3) + "/" + hash.substr(3, 3) + "/" + hash;
    // And drop the two-level directory to keep client complexity down since
    // xrootd seems to check raw paths.
    return DUMP_BASE + "/" + hash;

}

//////////////////////////////////////////////////////////////////////
// ScriptMeta
//////////////////////////////////////////////////////////////////////
qWorker::ScriptMeta::ScriptMeta(StringBuffer const& b, int chunkId_) {
    script = b.getStr();
    hash = hashQuery(script.data(), script.length());
    dbName = "q_" + hash;
    resultPath = hashToResultPath(hash);
    chunkId = chunkId_;
}

//////////////////////////////////////////////////////////////////////
// StringBuffer
//////////////////////////////////////////////////////////////////////
void qWorker::StringBuffer::addBuffer(
    XrdSfsFileOffset offset, char const* buffer, XrdSfsXferSize bufferSize) {
#if QSERV_USE_STUPID_STRING
#  if DO_NOT_USE_BOOST
    UniqueLock lock(_mutex);
#  else
    boost::unique_lock<boost::mutex> lock(_mutex);
#  endif
    _ss << std::string(buffer,bufferSize);
    _totalSize += bufferSize;

#else
    char* newItem = new char[bufferSize];
    assert(newItem != (char*)0);
    memcpy(newItem, buffer, bufferSize);
    { // Assume(!) that there are no overlapping writes.
#  if DO_NOT_USE_BOOST
	UniqueLock lock(_mutex);
#  else
	boost::unique_lock<boost::mutex> lock(_mutex);
#  endif
	_buffers.push_back(Fragment(offset, newItem, bufferSize));
	_totalSize += bufferSize;
    }
#endif
}

std::string qWorker::StringBuffer::getStr() const {
#if QSERV_USE_STUPID_STRING
    // Cast away const in order to lock.
#if DO_NOT_USE_BOOST
    UniqueLock lock(const_cast<XrdSysMutex&>(_mutex));
#else
    boost::mutex& mutex = const_cast<boost::mutex&>(_mutex);
    boost::unique_lock<boost::mutex> lock(mutex);
#endif
    return _ss.str();
#else
    std::string accumulated;
    if(false) {
    // Cast away const to perform a sort (which doesn't logically change state)
    FragmentDeque& nonConst = const_cast<FragmentDeque&>(_buffers);
    std::sort(nonConst.begin(), nonConst.end(), offsetLess<Fragment>());
    }
    FragmentDeque::const_iterator bi; 
    FragmentDeque::const_iterator bend = _buffers.end(); 

    //    accumulated.assign(getLength(), '\0'); // 
    for(bi = _buffers.begin(); bi != bend; ++bi) {
	Fragment const& p = *bi;
	accumulated += std::string(p.buffer, p.bufferSize);
	// Perform "writes" of the buffers into the string
	// Assume that we end up with a contiguous string.
	//accumulated.replace(p.offset, p.bufferSize, p.buffer, p.bufferSize);
    }
    return accumulated;
#endif
}

std::string qWorker::StringBuffer::getDigest() const {  
#if QSERV_USE_STUPID_STRING
    // Cast away const in order to lock.
#if DO_NOT_USE_BOOST
    UniqueLock lock(const_cast<XrdSysMutex&>(_mutex));
#else
    boost::mutex& mutex = const_cast<boost::mutex&>(_mutex);
    boost::unique_lock<boost::mutex> lock(mutex);
#endif
    int length = 200;
    if(length > _totalSize) 
	length = _totalSize;
    
    return std::string(_ss.str().data(), length); 
#else
    FragmentDeque::const_iterator bi; 
    FragmentDeque::const_iterator bend = _buffers.end(); 

    std::stringstream ss;
    for(bi = _buffers.begin(); bi != bend; ++bi) {
	Fragment const& p = *bi;
	ss << "Offset=" << p.offset << "\n";
	int fragsize = 100;
	if(fragsize > p.bufferSize) fragsize = p.bufferSize;
	ss << std::string(p.buffer, fragsize) << "\n";
    }
    return ss.str();
#endif
}


XrdSfsFileOffset qWorker::StringBuffer::getLength() const {
    return _totalSize;
    // Might be wise to do a sanity check sometime (overlapping writes!)
#if 0
    struct accumulateSize {    
	XrdSfsXferSize operator() (XrdSfsFileOffset x, Fragment const& p) { 
	    return x + p.bufferSize; 
	}
    };
    return std::accumulate(_buffers.begin(), _buffers.end(), 
			   0, accumulateSize());
#endif
}


void qWorker::StringBuffer::reset() {
    {
#if DO_NOT_USE_BOOST
	UniqueLock lock(_mutex);
#else
	boost::unique_lock<boost::mutex> lock(_mutex);
#endif
	std::for_each(_buffers.begin(), _buffers.end(), ptrDestroy<Fragment>());
	_buffers.clear();
    }
}