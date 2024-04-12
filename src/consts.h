#ifndef CONSTS_H
#define CONSTS_H

const char *const PLUGIN_INFO_TEMPLATE =
	"<a href=\"https://github.com/occ-ai/obs-ocr/\">OCR Plugin</a> (%1) by "
	"<a href=\"https://github.com/occ-ai\">OCC AI</a> ❤️ "
	"<a href=\"https://www.patreon.com/RoyShilkrot\">Support & Follow</a>";

const char *const WHITELIST_CHARS_ENGLISH =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+-=[]{}|;':\",./<>?`~\\ ";
// add french characters with accents
const char *const WHITELIST_CHARS_FRENCH =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+-=[]{}|;':\",./<>?`~\\éèêàâùûç ";
// add german characters with umlauts
const char *const WHITELIST_CHARS_GERMAN =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+-=[]{}|;':\",./<>?`~\\äöüß ";
// add spanish characters with accents
const char *const WHITELIST_CHARS_SPANISH =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+-=[]{}|;':\",./<>?`~\\áéíóúüñ ";
// add italian characters with accents
const char *const WHITELIST_CHARS_ITALIAN =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+-=[]{}|;':\",./<>?`~\\àèéìòù ";
// add portuguese characters with accents
const char *const WHITELIST_CHARS_PORTUGUESE =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+-=[]{}|;':\",./<>?`~\\áàãâéêíóôõúüç ";
// add russian characters for cyrillic, both upper and lower case
const char *const WHITELIST_CHARS_RUSSIAN =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+-=[]{}|;':\",./<>?`~\\абвгдеёжзийклмнопрстуфхцчшщъыьэюя ";
const char *const WHITELIST_CHARS_JAPANESE =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+-=[]{}|;':\",./<>?`~\\あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよらりるれろわをん ";
// hindi characters
const char *const WHITELIST_CHARS_HINDI =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+-=[]{}|;':\",./<>?`~\\अआइईउऊऋएऐओऔकखगघङचछजझञटठडढणतथदधनपफबभमयरलवशषसह ";
// arabic characters
const char *const WHITELIST_CHARS_ARABIC =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+-=[]{}|;':\",./<>?`~\\ا ب ت ث ج ح خ د ذ ر ز س ش ص ض ط ظ ع غ ف ق ك ل م ن ه و ي ";
// numeric characters with punctuation for time, date, currency, etc.
const char *const WHITELIST_CHARS_NUMERIC = "0123456789!@#$%^&*()_+-=[]{}|;':\",./<>?`~\\ ";

const int OUTPUT_IMAGE_OPTION_DETECTION_MASK = 0;
const int OUTPUT_IMAGE_OPTION_TEXT_OVERLAY = 1;
const int OUTPUT_IMAGE_OPTION_TEXT_BACKGROUND = 2;

#endif /* CONSTS_H */
