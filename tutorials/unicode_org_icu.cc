//
// Created by admin on 2022/7/13.
//

#include <unicode/unistr.h>
#include <unicode/ustdio.h>
#include <unicode/brkiter.h>

void PrintUnicodeString(UFILE* out, const icu::UnicodeString& s) {
    icu::UnicodeString other = s;
    u_fprintf(out, "\"%S\"", other.getTerminatedBuffer());
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    UFILE* out;
    UErrorCode status = U_ZERO_ERROR;
    out = u_finit(stdout, nullptr, nullptr);
    if (!out) {
        fprintf(stderr, "Could not initialize over stdout! \n");
        return 1;
    }

    ucnv_setFromUCallBack(u_fgetConverter(out),
                          UCNV_FROM_U_CALLBACK_ESCAPE,
                          nullptr, nullptr, nullptr, &status);
    if (U_FAILURE(status)) {
        u_fprintf(out, "Warning- couldn't set the substitute callback - err %s\n",
                  u_errorName(status));
    }

    u_fprintf(out, "ICU Case Mapping Sample Program\n\n");
    u_fprintf(out, "C++ Case Mapping\n\n");

    icu::UnicodeString string("This is a test");
    UChar lowercase[] = {0x69, 0x73, 0x74, 0x61, 0x6e, 0x62, 0x75, 0x6c, 0};
    UChar uppercase[] = {0x0130, 0x53, 0x54, 0x41, 0x4e, 0x42, 0x55, 0x4C, 0};

    icu::UnicodeString upper(uppercase);
    icu::UnicodeString lower(lowercase);

    u_fprintf(out, "\nstring: ");
    PrintUnicodeString(out, string);
    string.toUpper();
    u_fprintf(out, "\ntoUpper(): ");
    PrintUnicodeString(out, string);
    string.toLower();
    u_fprintf(out, "\ntoLower(): ");
    PrintUnicodeString(out, string);

    u_fprintf(out, "\n\nlowercase=%S, uppercase=%S\n", lowercase, uppercase);

    string = upper;
    // Turkish lower case map string = lowercase
    string.toLower(icu::Locale("tr", "TR"));
    u_fprintf(out, "\nupper.toLower: ");
    PrintUnicodeString(out, string);

    string = lower;
    string.toUpper(icu::Locale("tr", "TR"));
    u_fprintf(out, "\nlower.toUpper: ");
    PrintUnicodeString(out, string);

    u_fclose(out);
    return 0;
}
