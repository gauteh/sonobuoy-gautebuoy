"""
Gaute Hope <eg@gaute.vetsj.com> (c) 2011-09-09

arduino.py: Interface and protocol to Arduino board

"""

from synapse import *
from synapse.evalBase import *
from synapse.switchboard import *

from gps import *

ARDUINO_UART  = 1
ARDUINO_BAUD  = 38400

def a_setup ():
  initUart (ARDUINO_UART, ARDUINO_BAUD, 8, 'N', 1)
  stdinMode (1, False)

  crossConnect (DS_UART1, DS_STDIO)

# Receiver state:
# 0 = Waiting for '$'
# 1 = Between '$' and '*'
# 2 = On first CS digit
# 3 = On second CS digit
a_receive_state = 0
a_buf   = ''

def a_stdinhandler (buf):
  global token, tokeni
  global a_receive_state, a_buf

  i = 0
  l = len (buf)

  while (i < l):
    c = buf[i]
    i += 1

    if (a_receive_state == 0):
      if (c == '$'):
        a_buf = '$'
        a_receive_state = 1

    elif (a_receive_state == 1):
      if (c == '*'):
        a_receive_state = 2

      a_buf += c

    elif (a_receive_state == 2):
      a_buf += c
      a_receive_state = 3

    elif (a_receive_state == 3):
      a_buf += c
      a_parse (a_buf)
      a_buf = ''
      a_receive_state = 0

    else:
      # Something went terribly wrong..
      a_buf = ''
      a_receive_state = 0

    # Check if we're receiving sane amounts of data..
    if len(a_buf) > 80:
      a_buf = ''
      a_receive_state = 0

def a_parse (buf):
  # Test checksum
  if (not test_checksum (buf)):
    print "[AUINO] Message discarded, checksum failed."
    return

  # Parse message
  tokeni = 0
  token  = ''

  msgtype = ''
  subtype = ''

  finished = False

  i = 0
  l = len (buf)

  while (i < l):
    # Get token
    token = ''
    while ((i < l) and (buf[i] != ',' and buf[i] != '*')):
      token += buf[i]
      i += 1

    if ((i < l) and buf[i] == '*'):
      finished = True

    i += 1 # Skip delimiter

    if (tokeni == 0):
      msgtype = token[1:]
    else:
      if (msgtype == 'GPS'):
        if (tokeni == 1): subtype = token
        elif (tokeni > 1):
          if (subtype == 'S'):
            if (tokeni == 2):
              global lasttype
              lasttype = token

            elif (tokeni == 3):
              global telegramsreceived
              telegramsreceived = token

            elif (tokeni == 4):
              global latitude
              latitude = token

            elif (tokeni == 5):
              global north
              north = (token[0] == 'N')

            elif (tokeni == 6):
              global longitude
              longitude = token

            elif (tokeni == 7):
              global east
              east = (token[0] == 'E')

            elif (tokeni == 8):
              global gps_time
              gps_time = token

            elif (tokeni == 9):
              global valid
              valid = (token == 'Y')

      elif (msgtype == 'AD'):
        if (tokeni == 1): subtype = token
        elif (tokeni > 1):
          if (subtype == 'S'):
            if (tokeni == 3):
              pass
            elif (tokeni == 4):
              pass
            elif (tokeni == 5):
              pass

      elif (msgtype == 'DBG'):
        if (tokeni == 1):
          print "[DBG] ", token

    tokeni += 1


def test_checksum (s):
  csum = s[-2:]
  i = 1
  l = len(s) - 3

  sum = 0
  while (i < l):
    sum = sum ^ ord(s[i])
    i += 1

  return (hex(sum) == csum)

''' Return hexdecimal string representation of integer, result in 2 digits '''
def hex (i):
  h = '0123456789ABCDEF'
  return h[i >> 4] + h[i & 0x0f]
