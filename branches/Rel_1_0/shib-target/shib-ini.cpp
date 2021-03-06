/*
 * shib-ini.h -- INI file handling
 *
 * Created By:	Derek Atkins <derek@ihtfp.com>
 *
 * $Id$
 */

// eventually we might be able to support autoconf via cygwin...
#if defined (_MSC_VER) || defined(__BORLANDC__)
# include "config_win32.h"
#else
# include "config.h"
#endif

#include "shib-target.h"
#include <shib/shib-threads.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <log4cpp/Category.hh>

using namespace std;
using namespace shibboleth;
using namespace shibtarget;

class HeaderIterator : public shibtarget::ShibINI::Iterator {
public:
  HeaderIterator (ShibINIPriv* ini);
  ~HeaderIterator ();

  const string* begin();
  const string* next();
private:
  ShibINIPriv* ini;
  map<string, map<string,string> >::const_iterator iter;
  bool valid;
};

class TagIterator : public ShibINI::Iterator {
public:
  TagIterator (ShibINIPriv* ini, const string& header);
  ~TagIterator ();

  const string* begin();
  const string* next();
private:
  ShibINIPriv* ini;
  string header;
  map<string,string>::const_iterator iter;
  bool valid;
};

class shibtarget::ShibINIPriv {
public:
  ShibINIPriv();
  ~ShibINIPriv() { delete rwlock; }
  log4cpp::Category *log;

  map<string, map<string, string> > table;
  string file;
  bool cs;

  unsigned long	modtime;

  unsigned long iterators;
  RWLock *rwlock;

  bool exists(const std::string& header);
  bool exists(const std::string& header, const std::string& tag);
};

ShibINIPriv::ShibINIPriv()
{
  string ctx = "shibtarget.ShibINI";
  log = &(log4cpp::Category::getInstance(ctx));
  rwlock = RWLock::create();
  modtime = 0;
  iterators = 0;
}

static void trimline (string& s)
{
  int end = s.size() - 1, start = 0;

  // Trim stuff on right.
  while (end > 0 && !isgraph(s[end])) end--;

  // Trim stuff on left.
  while (start < end && !isgraph(s[start])) start++;

  // Modify the string.
  s = s.substr(start, end - start + 1);
}

static void to_lowercase (string& s)
{
  for (int i = 0, sz = s.size(); i < sz; i++)
    s[i] = tolower(s[i]);
}

ShibINI::~ShibINI() {
  delete m_priv;
}

void ShibINI::init (string& f, bool case_sensitive)
{
  m_priv = new ShibINIPriv();
  m_priv->file = f;
  m_priv->cs = case_sensitive;
  m_priv->log->info ("initializing INI file: %s (sensitive=%s)", f.c_str(),
		     (case_sensitive ? "true" : "false"));

  ReadLock lock(m_priv->rwlock);
  refresh();
}

//
// Must be called holding the ReadLock.
//
void ShibINI::refresh(void)
{
  saml::NDC ndc("refresh");

  // check if we need to refresh
#ifdef _WIN32
  struct _stat stat_buf;
  if (_stat (m_priv->file.c_str(), &stat_buf) < 0)
#else
  struct stat stat_buf;
  if (stat (m_priv->file.c_str(), &stat_buf) < 0)
#endif
    m_priv->log->error("stat failed: %s", m_priv->file.c_str());

#ifdef DEBUG
  m_priv->log->info("refresh: last modtime at %d; file is %d; iters: %d",
		    m_priv->modtime, stat_buf.st_mtime, m_priv->iterators);
#endif

  if (m_priv->modtime >= stat_buf.st_mtime || m_priv->iterators > 0)
    return;

  // Release the read lock -- grab the write lock.  Don't worry if
  // this is non-atomic -- we'll recheck the status.
  m_priv->rwlock->unlock();
  m_priv->rwlock->wrlock();

  // Recheck the modtime
  if (m_priv->modtime >= stat_buf.st_mtime) {
    // Yep, another thread got to it.  We can exit now...  Release
    // the write lock and reaquire the read-lock.

    m_priv->rwlock->unlock();
    m_priv->rwlock->rdlock();
    return;
  }

  // Ok, we've got the write lock.  Let's update our state.

  m_priv->modtime = stat_buf.st_mtime;

  // clear the existing maps
  m_priv->table.clear();

  m_priv->log->info("reading %s", m_priv->file.c_str());
  
  // read the file
  try
  {
    ifstream infile (m_priv->file.c_str());
    if (!infile) {
      m_priv->log->warn("cannot open file: %s", m_priv->file.c_str());
      m_priv->rwlock->unlock();
      m_priv->rwlock->rdlock();
      return;
    }

    const int MAXLEN = 1024;
    char linebuffer[MAXLEN];
    string current_header;
    bool have_header = false;

    while (infile) {
      infile.getline (linebuffer, MAXLEN);
      string line (linebuffer);

      if (line[0] == '#') continue;

      trimline (line);
      if (line.size() <= 1) continue;

      if (line[0] == '[') {
	// this is a header

#ifdef DEBUG
	m_priv->log->info("Found what appears to be a header line");
#endif

	have_header = false;

	// find the end of the header
	int endpos = line.find (']');
	if (endpos == line.npos) {
#ifdef DEBUG
	  m_priv->log->info("Weird: no end found.. punting");
#endif
	  continue; // HUH?  No end?
	}

	// found it
	current_header = line.substr (1, endpos-1);
	trimline (current_header);

	if (!m_priv->cs) to_lowercase (current_header);

	m_priv->table[current_header] = map<string,string>();
	have_header = true;
#ifdef DEBUG
	m_priv->log->info("current header: \"%s\"", current_header.c_str());
#endif

      } else if (have_header) {
	// this is a tag

#ifdef DEBUG
	m_priv->log->info("Found what appears to be a tag line");
#endif

	string tag, setting;
	int mid = line.find ('=');

	if (mid == line.npos) {
#ifdef DEBUG
	  m_priv->log->info("Weird: no '=' found.. punting");
#endif
	  continue; // Can't find the value's setting
	}

	tag = line.substr (0,mid);
	setting = line.substr (mid+1, line.size()-mid);

	trimline (tag);
	trimline (setting);

	if (!m_priv->cs) to_lowercase (tag);

	// If it already exists, log an error and do not save it
	if (m_priv->exists (current_header, tag))
	  m_priv->log->error("Duplicate tag found in section %s: \"%s\"",
			     current_header.c_str(), tag.c_str());
	else
	  (m_priv->table[current_header])[tag] = setting;

#ifdef DEBUG
	m_priv->log->info("new tag: \"%s\" = \"%s\"",
			  tag.c_str(), setting.c_str());
#endif

      }

    } // until the file ends

  } catch (...) {
    // In case there are exceptions.
  }

  // Now release the write lock and reaquire the read lock
  m_priv->rwlock->unlock();
  m_priv->rwlock->rdlock();
}

const std::string ShibINI::get (const string& header, const string& tag)
{
  ReadLock rwlock(m_priv->rwlock);
  refresh();

  static string empty = "";

  string h = header;
  string t = tag;

  if (!m_priv->cs) {
    to_lowercase (h);
    to_lowercase (t);
  }

  if (!m_priv->exists(h)) return empty;

  map<string,string>::const_iterator i = m_priv->table[h].find(t);
  if (i == m_priv->table[h].end())
    return empty;
  return i->second;
}

bool ShibINIPriv::exists(const std::string& header)
{
  string h = header;
  if (!cs) to_lowercase (h);

  return (table.find(h) != table.end());
}

bool ShibINI::exists(const std::string& header)
{
  ReadLock rwlock(m_priv->rwlock);
  refresh();

  return m_priv->exists(header);
}

bool ShibINIPriv::exists(const std::string& header, const std::string& tag)
{
  string h = header;
  string t = tag;

  if (!cs) {
    to_lowercase (h);
    to_lowercase (t);
  }

  if (!exists(h)) return false;
  return (table[h].find(t) != table[h].end());
}

bool ShibINI::exists(const std::string& header, const std::string& tag)
{
  ReadLock rwlock(m_priv->rwlock);
  refresh();

  return m_priv->exists(header, tag);
}

bool ShibINI::get_tag (string& header, string& tag, bool try_general, string* result)
{
  if (!result) return false;

  m_priv->rwlock->rdlock();
  refresh();
  m_priv->rwlock->unlock();

  if (m_priv->exists (header, tag)) {
    *result = get (header, tag);
    return true;
  }
  if (try_general && exists (SHIBTARGET_GENERAL, tag)) {
    *result = get (SHIBTARGET_GENERAL, tag);
    return true;
  }
  return false;
}


void ShibINI::dump (ostream& os)
{
  ReadLock rwlock(m_priv->rwlock);
  refresh();

  os << "File: " << m_priv->file << "\n";
  os << "Case-Sensitive: " << ( m_priv->cs ? "Yes\n" : "No\n" );
  os << "File Entries:\n";

  for (map<string, map<string, string> >::const_iterator i = m_priv->table.begin();
       i != m_priv->table.end(); i++) {

    os << "[" << i->first << "]\n";

    for (map<string,string>::const_iterator j=i->second.begin();
	 j != i->second.end(); j++) {

      os << "  " << j->first << " = " << j->second << "\n";
    }
  }

  os << "END\n";
}

ShibINI::Iterator* ShibINI::header_iterator()
{
  ReadLock rwlock(m_priv->rwlock);
  refresh();
  HeaderIterator* iter = new HeaderIterator(m_priv);
  return (ShibINI::Iterator*) iter;
}

ShibINI::Iterator* ShibINI::tag_iterator(const std::string& header)
{
  ReadLock rwlock(m_priv->rwlock);
  refresh();
  TagIterator* iter = new TagIterator(m_priv, header);
  return (ShibINI::Iterator*) iter;
}

//
// XXX: FIXME: there may be a race condition in the iterators if a
// caller holds an active Iterator, the underlying file changes, and
// then calls one of the get() routines.  It's possible the iterator
// may screw up -- I don't know whether the iterator actually depends
// on the underlying infrastructure or not.
//

HeaderIterator::HeaderIterator (ShibINIPriv* inip)
{
  saml::NDC ndc("HeaderIterator");
  ini = inip;
  valid = false;
  ini->rwlock->rdlock();
  ini->iterators++;
  ini->log->debug("iterators: %d", ini->iterators);
}

HeaderIterator::~HeaderIterator ()
{
  saml::NDC ndc("~HeaderIterator");
  ini->iterators--;
  ini->rwlock->unlock();
  ini->log->debug("iterators: %d", ini->iterators);
}

const string* HeaderIterator::begin ()
{
  iter = ini->table.begin();
  if (iter == ini->table.end()) {
    valid = false;
    return 0;
  }
  valid = true;
  return &iter->first;
}

const string* HeaderIterator::next ()
{
  if (!valid)
    return 0;
  iter++;
  if (iter == ini->table.end()) {
    valid = false;
    return 0;
  }
  return &iter->first;
}

TagIterator::TagIterator (ShibINIPriv* inip, const string& headerp)
  : header(headerp)
{
  saml::NDC ndc("TagIterator");
  ini = inip;
  valid = false;
  ini->rwlock->rdlock();
  ini->iterators++;
  ini->log->debug("iterators: %d", ini->iterators);
}

TagIterator::~TagIterator ()
{
  saml::NDC ndc("~TagIterator");
  ini->iterators--;
  ini->rwlock->unlock();
  ini->log->debug("iterators: %d", ini->iterators);
}

const string* TagIterator::begin ()
{
  iter = ini->table[header].begin();
  if (iter == ini->table[header].end()) {
    valid = false;
    return 0;
  }
  valid = true;
  return &iter->first;
}

const string* TagIterator::next ()
{
  if (!valid)
    return 0;
  iter++;
  if (iter == ini->table[header].end()) {
    valid = false;
    return 0;
  }
  return &iter->first;
}

bool ShibINI::boolean(string& value)
{
  const char* v = value.c_str();
#ifdef HAVE_STRCASECMP
  if (!strncasecmp (v, "on", 2) || !strncasecmp (v, "true", 4) || !strncmp(v, "1", 1))
    return true;
#else
  if (!strnicmp (v, "on", 2) || !strnicmp (v, "true", 4) || !strncmp(v, "1", 1))
    return true;
#endif
  return false;
}

ShibINI::Iterator::~Iterator() {}
