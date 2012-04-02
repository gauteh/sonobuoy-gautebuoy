"""
Gaute Hope <eg@gaute.vetsj.com> (c) 2011-09-09

arduino.py: Interface and protocol to Arduino board

"""

from util import *

class Protocol:
  zero = None
  logger = None

  def __init__ (self, z):
    self.zero     = z
    self.logger   = z.logger

  # Last type received
  lasttype = ''

  # Receiver state:
  # 0 = Waiting for '$'
  # 1 = Between '$' and '*'
  # 2 = On first CS digit
  # 3 = On second CS digit
  # 4 = Waiting for '$' to signal start of binary data
  # 5 = Receiving AD binary sample data

  a_receive_state = 0
  a_buf           = ''
  waitforreceipt  = False # Have just got a AD data batch and is waiting for
                          # a DE receipt message

  def handle (self, buf):
    i = 0
    l = len (buf)

    while (i < l):
      c = buf[i]
      i += 1

      if (self.a_receive_state == 0):
        if (c == '$'):
          self.a_buf = '$'
          self.a_receive_state = 1

      elif (self.a_receive_state == 1):
        if (c == '*'):
          self.a_receive_state = 2

        self.a_buf += c

      elif (self.a_receive_state == 2):
        self.a_buf += c
        self.a_receive_state = 3

      elif (self.a_receive_state == 3):
        self.a_buf += c
        self.a_receive_state = 0
        self.a_parse (self.a_buf)
        self.a_buf = ''

      elif (self.a_receive_state == 4):
        if (c == '$'):
          self.a_receive_state = 5

      elif (self.a_receive_state == 5):
        self.zero.current.ad.ad_samples += c
        self.zero.current.ad.ad_k_remaining -= 1
        if (self.zero.current.ad.ad_k_remaining < 1):
          self.a_receive_state = 0
          self.waitforreceipt = True

      else:
        # Something went terribly wrong..
        self.a_buf = ''
        self.a_receive_state = 0
        self.waitforreceipt = False

      # Check if we're receiving sane amounts of data..
      if len(self.a_buf) > 80:
        self.a_buf = ''
        self.a_receive_state = 0


  def a_parse (self, buf):
    # Test checksum
    if (not test_checksum (buf)):
      self.logger.info ("[Protocol] Message discarded, checksum failed.")
      self.logger.info ("[Protocol] Discarded: " + buf)
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
        if self.waitforreceipt:
          if msgtype != 'AD' or (tokeni > 1 and subtype != 'DE'):
            self.logger.error ("[Protocol] Did not receive receipt immediately after data batch. Discarding data batch.")
            self.waitforreceipt = False
            self.zero.current.ad.ad_k_samples = 0
            self.zero.current.ad.ad_reference = 0


        if (msgtype == 'GPS'):
          if (tokeni == 1): subtype = token
          elif (tokeni > 1):
            if (subtype == 'S'):
              if (tokeni == 2):
                self.zero.current.gps.lasttype = token

              elif (tokeni == 3):
                try:
                  self.zero.current.gps.telegramsreceived = int(token)
                except ValueError:
                  self.logger.exception ("[Protocol] Could not convert token to int. Discarding rest of message.")
                  return

              elif (tokeni == 4):
                try:
                  self.zero.current.gps.latitude = float(token) if len(token) else 0
                except ValueError:
                  self.logger.exception ("[Protocol] Could not convert token to float. Discarding rest of message.")
                  return

              elif (tokeni == 5):
                self.zero.current.gps.north = (token[0] == 'N')

              elif (tokeni == 6):
                try:
                  self.zero.current.gps.longitude = float(token) if len(token) > 0 else 0
                except ValueError:
                  self.logger.exception ("[Protocol] Could not convert token to float. Discarding rest of message.")
                  return

              elif (tokeni == 7):
                self.zero.current.gps.east = (token[0] == 'E')

              elif (tokeni == 8):
                self.zero.current.gps.unix_time = token

              elif (tokeni == 9):
                self.zero.current.gps.gps_time = token

              elif (tokeni == 10):
                self.zero.current.gps.gps_date = token

              elif (tokeni == 11):
                self.zero.current.gps.valid = (token == 'Y')

              elif (tokeni == 12):
                self.zero.current.gps.has_time = (token == 'Y')

              elif (tokeni == 13):
                self.zero.current.gps.has_sync = (token == 'Y')

              elif (tokeni == 14):
                self.zero.current.gps.has_sync_reference = (token == 'Y')

                self.zero.current.gps.gps_status ()

        elif (msgtype == 'AD'):
          if (tokeni == 1): subtype = token
          elif (tokeni > 1):
            if (subtype == 'S'):
              try:
                if (tokeni == 2):
                  self.zero.current.ad.ad_qposition = int(token)
                elif (tokeni == 3):
                  self.zero.current.ad.ad_queue_time = int (token)
                elif (tokeni == 4):
                  self.zero.current.ad.ad_value = token
                elif (tokeni == 5):
                  self.zero.current.ad.ad_config = token
                  self.zero.current.ad.ad_status ()
                  return
              except ValueError:
                self.logger.exception ("[Protocol] Could not convert token to int. Discarding rest of message.")
                return

            elif (subtype == 'D'):
              if (tokeni == 2):
                try:
                  self.zero.current.ad.ad_k_samples = int (token)
                except ValueError:
                  self.logger.exception ("[Protocol] Could not convert token to int. Discarding rest of message.")
                  return

                if (self.zero.current.ad.ad_k_samples > self.zero.current.ad.AD_K_SAMPLES_MAX):
                  self.logger.error ("[Protocol] Too large batch, resetting protocol.")
                  self.a_receive_state = 0
                else:

                  self.zero.current.ad.ad_k_remaining = self.zero.current.ad.ad_k_samples * 4
                  self.zero.current.ad.ad_samples = ''
                  self.a_receive_state = 4

              elif (tokeni == 3):
                try:
                  self.zero.current.ad.ad_reference = int (token)
                except ValueError:
                  self.logger.exception ("[Protocol] Could not convert token to int. Discarding rest of message.")
                  return

              elif (tokeni == 4):
                try:
                  self.zero.current.ad.ad_reference_status = int (token)
                except ValueError:
                  self.logger.exception ("[Protocol] Could not convert token to int. Discarding rest of message.")
                  return

                #print "[AD] Initiating binary transfer.. samples: ", self.zero.current.ad.ad_k_samples
                return

            elif (subtype == 'DE'):
              if not self.waitforreceipt:
                self.logger.error ("[Protocol] Got end of batch data without getting data first.")
                self.zero.current.ad.ad_k_samples = 0
                self.zero.current.ad.ad_reference = 0
                self.waitforreceipt = False
                return

              if (tokeni == 2):
                #print "[AD] Binay data transfer complete."
                self.zero.current.ad.ad_sample_csum = token
                self.zero.current.ad.ad_handle_samples ()
                self.waitforreceipt = False

              return

        elif (msgtype == 'DBG'):
          if (tokeni == 1):
            self.logger.info ("[Buoy] " + token)

      tokeni += 1

