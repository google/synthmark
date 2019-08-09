#!/usr/bin/python2.7
#
# Copyright 2009 Google Inc. All Rights Reserved.

"""Interface for a USB-connected Monsoon power meter
(http://msoon.com/LabEquipment/PowerMonitor/).

"""

_author_ = 'kens@google.com (Ken Shirriff)'

import fcntl
import os
import select
import signal
import stat
import struct
import sys
import time
import collections

try:
  from absl import flags
except ImportError:
  import gflags as flags  # http://code.google.com/p/python-gflags/

try:
  from google3.third_party.py import serial
except ImportError:
  import serial           # http://pyserial.sourceforge.net/

FLAGS = flags.FLAGS

class Monsoon:
  """
  Provides a simple class to use the power meter, e.g.
  mon = monsoon.Monsoon()
  mon.SetVoltage(3.7)
  mon.StartDataCollection()
  mydata = []
  while len(mydata) < 1000:
    mydata.extend(mon.CollectData())
  mon.StopDataCollection()

  See http://wiki/Main/MonsoonProtocol for information on the protocol.
  """

  USB_OFF = 0
  USB_ON = 1
  USB_AUTO = 2

  def __init__(self, device=None, serialno=None, wait=1):
    """
    Establish a connection to a Monsoon.
    By default, opens the first available port, waiting if none are ready.
    A particular port can be specified with "device", or a particular Monsoon
    can be specified with "serialno" (using the number printed on its back).
    With wait=0, IOError is thrown if a device is not immediately available.
    """

    self._coarse_ref = self._fine_ref = self._coarse_zero = self._fine_zero = 0
    self._coarse_scale = self._fine_scale = 0
    self._last_seq = 0
    self.start_voltage = 0
    self._usb_passthrough = Monsoon.USB_AUTO

    if device:
      self.ser = serial.Serial(device, timeout=1)
      return

    while 1:  # try all /dev/ttyACM* until we find one we can use
      for dev in os.listdir("/dev"):
        if not dev.startswith("ttyACM"): continue
        tmpname = "/tmp/monsoon.%s.%s" % (os.uname()[0], dev)
        self._tempfile = open(tmpname, "w")
        try:
          os.chmod(tmpname, 0666)
        except OSError:
          pass
        try:  # use a lockfile to ensure exclusive access
          fcntl.lockf(self._tempfile, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except IOError as e:
          print >>sys.stderr, "device %s is in use" % dev
          continue

        try:  # try to open the device
          self.ser = serial.Serial("/dev/%s" % dev, timeout=1)
          self.StopDataCollection()  # just in case
          self._FlushInput()  # discard stale input
          status = self.GetStatus()
        except Exception as e:
          print >>sys.stderr, "error opening device %s: %s" % (dev, e)
          continue

        if not status:
          print >>sys.stderr, "no response from device %s" % dev
        elif serialno and status["serialNumber"] != serialno:
          print >>sys.stderr, ("Note: another device serial #%d seen on %s" %
                               (status["serialNumber"], dev))
        else:
          self.start_voltage = status["voltage1"]
          return

      self._tempfile = None
      if not wait: raise IOError("No device found")
      print >>sys.stderr, "waiting for device..."
      time.sleep(1)


  def GetStatus(self):
    """ Requests and waits for status.  Returns status dictionary. """

    # status packet format
    STATUS_FORMAT = ">BBBhhhHhhhHBBBxBbHBHHHHBbbHHBBBbbbbbbbbbBH"
    STATUS_FIELDS = [
        "packetType", "firmwareVersion", "protocolVersion",
        "mainFineCurrent", "usbFineCurrent", "auxFineCurrent", "voltage1",
        "mainCoarseCurrent", "usbCoarseCurrent", "auxCoarseCurrent", "voltage2",
        "outputVoltageSetting", "temperature", "status", "leds",
        "mainFineResistor", "serialNumber", "sampleRate",
        "dacCalLow", "dacCalHigh",
        "powerUpCurrentLimit", "runTimeCurrentLimit", "powerUpTime",
        "usbFineResistor", "auxFineResistor",
        "initialUsbVoltage", "initialAuxVoltage",
        "hardwareRevision", "temperatureLimit", "usbPassthroughMode",
        "mainCoarseResistor", "usbCoarseResistor", "auxCoarseResistor",
        "defMainFineResistor", "defUsbFineResistor", "defAuxFineResistor",
        "defMainCoarseResistor", "defUsbCoarseResistor", "defAuxCoarseResistor",
        "eventCode", "eventData", ]

    self._SendStruct("BBB", 0x01, 0x00, 0x00)
    while 1:  # Keep reading, discarding non-status packets
      bytes = self._ReadPacket()
      if not bytes: return None
      if len(bytes) != struct.calcsize(STATUS_FORMAT) or bytes[0] != "\x10":
        print >>sys.stderr, "wanted status, dropped type=0x%02x, len=%d" % (
                ord(bytes[0]), len(bytes))
        continue

      status = dict(zip(STATUS_FIELDS, struct.unpack(STATUS_FORMAT, bytes)))
      assert status["packetType"] == 0x10
      for k in status.keys():
        if k.endswith("VoltageSetting"):
          status[k] = 2.0 + status[k] * 0.01
        elif k.endswith("FineCurrent"):
          pass # needs calibration data
        elif k.endswith("CoarseCurrent"):
          pass # needs calibration data
        elif k.startswith("voltage") or k.endswith("Voltage"):
          status[k] = status[k] * 0.000125
        elif k.endswith("Resistor"):
          status[k] = 0.05 + status[k] * 0.0001
          if k.startswith("aux") or k.startswith("defAux"): status[k] += 0.05
        elif k.endswith("CurrentLimit"):
          status[k] = 8 * (1023 - status[k]) / 1023.0
      return status

  def RampVoltage(self, start, end):
    v = start
    if v < 3.0: v = 3.0       # protocol doesn't support lower than this
    while (v < end):
      self.SetVoltage(v)
      v += .1
      time.sleep(.1)
    self.SetVoltage(end)

  def SetVoltage(self, v):
    """ Set the output voltage, 0 to disable. """
    if v == 0:
      self._SendStruct("BBB", 0x01, 0x01, 0x00)
    else:
      self._SendStruct("BBB", 0x01, 0x01, int((v - 2.0) * 100))


  def SetMaxCurrent(self, i):
    """Set the max output current."""
    assert i >= 0 and i <= 8

    val = 1023 - int((i/8)*1023)
    self._SendStruct("BBB", 0x01, 0x0a, val & 0xff)
    self._SendStruct("BBB", 0x01, 0x0b, val >> 8)

  def SetMaxPowerUpCurrent(self, i):
    """Set the max power up current."""
    assert i >= 0 and i <= 8

    val = 1023 - int((i/8)*1023)
    self._SendStruct("BBB", 0x01, 0x08, val & 0xff)
    self._SendStruct("BBB", 0x01, 0x09, val >> 8)

  def SetUsbPassthrough(self, val):
    """ Set the USB passthrough mode: 0 = off, 1 = on,  2 = auto. """
    if val not in [Monsoon.USB_OFF, Monsoon.USB_ON, Monsoon.USB_AUTO]:
      raise Error("Expected one of: USB_OFF, USB_ON, USB_AUTO")

    self._usb_passthrough = val
    self._SendStruct("BBB", 0x01, 0x10, val)


  def GetUsbPassthrough(self):
    """ Return the USB passthrough mode (USB_OFF, USB_ON, USB_AUTO). """
    return self._usb_passthrough


  def StartDataCollection(self):
    """ Tell the device to start collecting and sending measurement data. """
    self._SendStruct("BBB", 0x01, 0x1b, 0x01) # Mystery command
    self._SendStruct("BBBBBBB", 0x02, 0xff, 0xff, 0xff, 0xff, 0x03, 0xe8)


  def StopDataCollection(self):
    """ Tell the device to stop collecting measurement data. """
    self._SendStruct("BB", 0x03, 0x00) # stop


  def CollectData(self):
    """ Return some current samples.  Call StartDataCollection() first. """
    while 1:  # loop until we get data or a timeout
      bytes = self._ReadPacket()
      if not bytes: return None
      if len(bytes) < 4 + 8 + 1 or bytes[0] < "\x20" or bytes[0] > "\x2F":
        print >>sys.stderr, "wanted data, dropped type=0x%02x, len=%d" % (
            ord(bytes[0]), len(bytes))
        continue

      seq, type, x, y = struct.unpack("BBBB", bytes[:4])
      data = [struct.unpack(">hhhh", bytes[x:x+8])
              for x in range(4, len(bytes) - 8, 8)]

      if self._last_seq and seq & 0xF != (self._last_seq + 1) & 0xF:
        print >>sys.stderr, "data sequence skipped, lost packet?"
      self._last_seq = seq

      if type == 0:
        if not self._coarse_scale or not self._fine_scale:
          print >>sys.stderr, "waiting for calibration, dropped data packet"
          continue

        out = []
        for main, usb, aux, voltage in data:
          if main & 1:
            out.append(((main & ~1) - self._coarse_zero) * self._coarse_scale)
          else:
            out.append((main - self._fine_zero) * self._fine_scale)
        return out

      elif type == 1:
        self._fine_zero = data[0][0]
        self._coarse_zero = data[1][0]
        # print >>sys.stderr, "zero calibration: fine 0x%04x, coarse 0x%04x" % (
        #     self._fine_zero, self._coarse_zero)

      elif type == 2:
        self._fine_ref = data[0][0]
        self._coarse_ref = data[1][0]
        # print >>sys.stderr, "ref calibration: fine 0x%04x, coarse 0x%04x" % (
        #     self._fine_ref, self._coarse_ref)

      else:
        print >>sys.stderr, "discarding data packet type=0x%02x" % type
        continue

      # See http://wiki/Main/MonsoonProtocol for the origin of these values
      if self._coarse_ref != self._coarse_zero:
        self._coarse_scale = 2.88 / (self._coarse_ref - self._coarse_zero)
      if self._fine_ref != self._fine_zero:
        self._fine_scale = 0.0332 / (self._fine_ref - self._fine_zero)


  def _SendStruct(self, fmt, *args):
    """ Pack a struct (without length or checksum) and send it. """
    data = struct.pack(fmt, *args)
    data_len = len(data) + 1
    checksum = (data_len + sum(struct.unpack("B" * len(data), data))) % 256
    out = struct.pack("B", data_len) + data + struct.pack("B", checksum)
    self.ser.write(out)


  def _ReadPacket(self):
    """ Read a single data record as a string (without length or checksum). """
    len_char = self.ser.read(1)
    if not len_char:
      print >>sys.stderr, "timeout reading from serial port"
      return None

    data_len = struct.unpack("B", len_char)
    data_len = ord(len_char)
    if not data_len: return ""

    result = self.ser.read(data_len)
    if len(result) != data_len: return None
    body = result[:-1]
    checksum = (data_len + sum(struct.unpack("B" * len(body), body))) % 256
    if result[-1] != struct.pack("B", checksum):
      print >>sys.stderr, "invalid checksum from serial port"
      return None
    return result[:-1]

  def _FlushInput(self):
    """ Flush all read data until no more available. """
    self.ser.flush()
    flushed = 0
    while True:
      ready_r, ready_w, ready_x = select.select([self.ser], [], [self.ser], 0)
      if len(ready_x) > 0:
        print >>sys.stderr, "exception from serial port"
        return None
      elif len(ready_r) > 0:
        flushed += 1
        self.ser.read(1)  # This may cause underlying buffering.
        self.ser.flush()  # Flush the underlying buffer too.
      else:
        break
    if flushed > 0:
      print >>sys.stderr, "dropped >%d bytes" % flushed

def main(argv):
  """ Simple command-line interface for Monsoon."""
  useful_flags = ["voltage", "status", "usbpassthrough", "samples", "current",
          "startcurrent"]
  if not any(
      [FLAGS.voltage, FLAGS.status, FLAGS.usbpassthrough, FLAGS.samples, FLAGS.current, FLAGS.startcurrent]):
    print __doc__.strip()
    print FLAGS.main_module_help()

  if FLAGS.avg and FLAGS.avg < 0:
    print "--avg must be greater than 0"
    return

  mon = Monsoon(device=FLAGS.device, serialno=FLAGS.serialno)

  if FLAGS.voltage is not None:
    if FLAGS.ramp is not None:
      mon.RampVoltage(mon.start_voltage, FLAGS.voltage)
    else:
      mon.SetVoltage(FLAGS.voltage)

  if FLAGS.current is not None:
    mon.SetMaxCurrent(FLAGS.current)

  if FLAGS.status:
    items = sorted(mon.GetStatus().items())
    print "\n".join(["%s: %s" % item for item in items])

  if FLAGS.usbpassthrough:
    if FLAGS.usbpassthrough == 'off':
      mon.SetUsbPassthrough(0)
    elif FLAGS.usbpassthrough == 'on':
      mon.SetUsbPassthrough(1)
    elif FLAGS.usbpassthrough == 'auto':
      mon.SetUsbPassthrough(2)
    else:
      sys.exit('bad passthrough flag: %s' % FLAGS.usbpassthrough)

  if FLAGS.startcurrent is not None:
     mon.SetMaxPowerUpCurrent(FLAGS.startcurrent)

  if FLAGS.samples:
    # Make sure state is normal
    mon.StopDataCollection()
    status = mon.GetStatus()
    native_hz = status["sampleRate"] * 1000

    # Collect and average samples as specified
    mon.StartDataCollection()

    # In case FLAGS.hz doesn't divide native_hz exactly, use this invariant:
    # 'offset' = (consumed samples) * FLAGS.hz - (emitted samples) * native_hz
    # This is the error accumulator in a variation of Bresenham's algorithm.
    emitted = offset = 0
    collected = []
    history_deque = collections.deque() # past n samples for rolling average

    try:
      last_flush = time.time()
      while emitted < FLAGS.samples or FLAGS.samples == -1:
        # The number of raw samples to consume before emitting the next output
        need = (native_hz - offset + FLAGS.hz - 1) / FLAGS.hz
        if need > len(collected):     # still need more input samples
          samples = mon.CollectData()
          if not samples: break
          collected.extend(samples)
        else:
          # Have enough data, generate output samples.
          # Adjust for consuming 'need' input samples.
          offset += need * FLAGS.hz
          while offset >= native_hz:  # maybe multiple, if FLAGS.hz > native_hz
            this_sample = sum(collected[:need]) / need

            if FLAGS.timestamp: print int(time.time()),

            if FLAGS.avg:
              history_deque.appendleft(this_sample)
              if len(history_deque) > FLAGS.avg: history_deque.pop()
              print "%f %f" % (this_sample,
                               sum(history_deque) / len(history_deque))
            else:
              print "%f" % this_sample
            sys.stdout.flush()

            offset -= native_hz
            emitted += 1              # adjust for emitting 1 output sample
          collected = collected[need:]
          now = time.time()
          if now - last_flush >= 0.99:  # flush every second
            sys.stdout.flush()
            last_flush = now
    except KeyboardInterrupt:
      print >>sys.stderr, "interrupted"

    mon.StopDataCollection()


if __name__ == '__main__':
  # Define flags here to avoid conflicts with people who use us as a library
  flags.DEFINE_boolean("status", None, "Print power meter status")
  flags.DEFINE_integer("avg", None,
                       "Also report average over last n data points")
  flags.DEFINE_float("voltage", None, "Set output voltage (0 for off)")
  flags.DEFINE_float("current", None, "Set max output current")
  flags.DEFINE_float("startcurrent", None, "Set max power-up/inital current")
  flags.DEFINE_string("usbpassthrough", None, "USB control (on, off, auto)")
  flags.DEFINE_integer("samples", None, "Collect and print this many samples")
  flags.DEFINE_integer("hz", 5000, "Print this many samples/sec")
  flags.DEFINE_string("device", None, "Use this /dev/ttyACM... file")
  flags.DEFINE_integer("serialno", None, "Look for this Monsoon serial number")
  flags.DEFINE_boolean("timestamp", None,
                       "Also print integer (seconds) timestamp on each line")
  flags.DEFINE_boolean("ramp", True, "Gradually increase voltage to prevent "
                       "tripping Monsoon overvoltage")

  main(FLAGS(sys.argv))
