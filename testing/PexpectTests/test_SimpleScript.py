import pexpect
import pytest

def test_SimpleScriptInsertMode():
  with open("script.sh", "w") as f:
    f.write("echo hi\n");

  child = pexpect.spawn("./gsc script.sh --shell bash")
  child.expect(r"\$ ")

  assert child.send("a") == 1
  assert child.expect("e") == 0

  assert child.send("a") == 1
  assert child.expect("c") == 0

  assert child.send("a") == 1
  assert child.expect("h") == 0

  assert child.send("a") == 1
  assert child.expect("o") == 0

  assert child.send("a") == 1
  assert child.expect(" ") == 0

  assert child.send("a") == 1
  assert child.expect("h") == 0

  assert child.send("a") == 1
  assert child.expect("i") == 0

  assert child.send("a") == 1
  with pytest.raises(pexpect.exceptions.TIMEOUT):
    child.expect(".",timeout=1)

  child.sendcontrol(r"m")
  assert child.expect("\r\n") == 0

  assert child.expect("hi\r\n") == 0

  child.expect(r"\$ ")

  child.sendcontrol(r"m")
  assert child.expect("\n") == 0
  assert child.expect("Session Finished. Press Enter.") == 0
  assert child.expect("\n") == 0


  child.terminate()
