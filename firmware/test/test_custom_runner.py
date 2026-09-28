"""PlatformIO-Testrunner fuer die selbstgeschriebene Testsuite.

Die Suite in test_dsp/ bringt ihr eigenes main() mit und braucht kein Unity.
Dieser Runner uebersetzt ihre Ausgabezeilen in PlatformIO-Testfaelle, damit
`pio test -e native` dieselben Ergebnisse anzeigt wie `make -C firmware test`.

Erwartetes Format je Zeile:
    [OK ] <Name>            bestanden
    [FAIL] <Name>           fehlgeschlagen
"""

from platformio.test.result import TestCase, TestStatus
from platformio.test.runners.base import TestRunnerBase


class CustomTestRunner(TestRunnerBase):
    def on_testing_line_output(self, line):
        super().on_testing_line_output(line)

        text = line.strip()
        for marker, status in (("[OK ]", TestStatus.PASSED),
                               ("[FAIL]", TestStatus.FAILED)):
            if not text.startswith(marker):
                continue
            rest = text[len(marker):].strip()
            # Die Suite haengt an viele Zeilen " ist X, soll Y" an. Das ist die
            # Meldung, nicht der Name.
            name, _, message = rest.partition("  ")
            self.test_suite.add_case(TestCase(
                name=name.strip() or rest,
                status=status,
                message=message.strip() or None,
            ))
            return
