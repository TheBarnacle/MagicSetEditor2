//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <util/spell_checker.hpp>
#include <util/string.hpp>
#include <util/io/package_manager.hpp>

// ----------------------------------------------------------------------------- : Spell checker : construction

map<String,SpellCheckerP> SpellChecker::spellers;

SpellChecker* SpellChecker::get(const String& language) {
  SpellCheckerP& speller = spellers[language];
  if (!speller) {
    String local_dir  = package_manager.getDictionaryDir(true);
    String global_dir = package_manager.getDictionaryDir(false);
    String aff_path = language + _(".aff");
    String dic_path = language + _(".dic");
    if (wxFileExists(local_dir + aff_path) && wxFileExists(local_dir + dic_path)) {
      speller = make_intrusive<SpellChecker>((local_dir + aff_path).mb_str(), (local_dir + dic_path).mb_str());
    } else if (wxFileExists(global_dir + aff_path) && wxFileExists(global_dir + dic_path)) {
      speller = make_intrusive<SpellChecker>((global_dir + aff_path).mb_str(), (global_dir + dic_path).mb_str());
    } else {
      queue_message(MESSAGE_ERROR, _("Dictionary not found for language: ") + language);
    }
  }
  return speller.get();
}

SpellChecker* SpellChecker::get(const String& filename, const String& language) {
  SpellCheckerP& speller = spellers[filename + _(".") + language];
  if (!speller) {
    String prefix = package_manager.openFilenameFromPackage(nullptr, filename) + _(".");
    String local_dir  = package_manager.getDictionaryDir(true);
    String global_dir = package_manager.getDictionaryDir(false);
    String aff_path = language + _(".aff");
    String dic_path = language + _(".dic");
    if (wxFileExists(prefix + dic_path)) {
      if (wxFileExists(prefix + aff_path)) {
        speller = make_intrusive<SpellChecker>((prefix + aff_path).mb_str(), (prefix + dic_path).mb_str());
      } else if (wxFileExists(local_dir + aff_path)) {
        speller = make_intrusive<SpellChecker>((local_dir + aff_path).mb_str(), (prefix + dic_path).mb_str());
      } else if (wxFileExists(global_dir + aff_path)) {
        speller = make_intrusive<SpellChecker>((global_dir + aff_path).mb_str(), (prefix + dic_path).mb_str());
      }
    }
    if (!speller) {
      queue_message(MESSAGE_ERROR, _("Dictionary '") + filename + _("' not found for language: ") + language);
    }
  }
  return speller.get();
}

SpellChecker::SpellChecker(const char* aff_path, const char* dic_path)
  : Hunspell(aff_path,dic_path)
  , encoding(String(get_dic_encoding(), IF_UNICODE(wxConvLibc, wxSTRING_MAXLEN)))
{}

void SpellChecker::destroyAll() {
  spellers.clear();
}

// ----------------------------------------------------------------------------- : Spell checker : use

bool SpellChecker::convert_encoding(const String& word, CharBuffer& out) {
  // fix curly quotes, especially apstrophes
  String fixed;
  FOR_EACH_CONST(c,word) {
    if (c == LEFT_SINGLE_QUOTE || c == RIGHT_SINGLE_QUOTE) {
      fixed += _('\'');
    } else if (c == LEFT_DOUBLE_QUOTE || c == RIGHT_DOUBLE_QUOTE) {
      fixed += _('\"');
    } else if (c == 0x00C6) {
      // expand ligatures, TODO: put this in a better place
      fixed += _("Ae");
    } else if (c == 0x0132) {
      fixed += _("IJ");
    } else if (c == 0x0152) {
      fixed += _("Oe");
    } else if (c == 0xFB01) {
      fixed += _("fi");
    } else if (c == 0xFB02) {
      fixed += _("fl");
    } else {
      fixed += c;
    }
  }
  // convert encoding
  out = fixed.mb_str(encoding);
  if (!out || *out == '\0') {
    // If encoding fails we get an empty string, since the word was not empty this can never happen
    // words that can't be encoded are not in the dictionary, so they are wrong.
    return false;
  } else {
    return true;
  }
}

bool SpellChecker::spell(const String& word) {
  if (word.empty()) return true; // empty word is okay
  CharBuffer str;
  if (!convert_encoding(word,str)) return false;
  return Hunspell::spell(str);
}

void SpellChecker::suggest(const String& word, vector<String>& suggestions_out) {
  CharBuffer str;
  if (!convert_encoding(word,str)) return;
  // call Hunspell
  char** suggestions;
  int num_suggestions = Hunspell::suggest(&suggestions, str);
  // copy sugestions
  for (int i = 0 ; i < num_suggestions ; ++i) {
    suggestions_out.push_back(String(suggestions[i],encoding));
    free(suggestions[i]);
  }
  free(suggestions);
}
