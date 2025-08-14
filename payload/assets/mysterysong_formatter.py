import os
import subprocess
import wave

def RunCommand(command, echo=False, capture=False, input=None, check=True, env=None):
    result = subprocess.run(command, capture_output=(
        not echo), input=input, check=check, shell=True, text=True,)
    if capture and not check:
        return (result.stdout + result.stderr).strip(), result.returncode
    elif capture:
        return (result.stdout + result.stderr).strip()
    elif not check:
        return result.returncode
    else:
        return

os.chdir(os.path.dirname(__file__))
RunCommand("ffmpeg -i mysterysong.xm -ar 44100 -ac 2 -c:a pcm_s16le -map_metadata -1 -y mysterysong.wav")

with wave.open("mysterysong.wav", "r") as mysterysongWav:
    with open("mysterysong.raw", "wb") as mysterysongRaw:
        sample_rate = mysterysongWav.getframerate()
        channel_count = mysterysongWav.getnchannels()
        bytes_per_sample = mysterysongWav.getsampwidth()
        sample_count = mysterysongWav.getnframes()
        mysterysongRaw.write(sample_rate.to_bytes(4, byteorder="little"))
        mysterysongRaw.write(channel_count.to_bytes(4, byteorder="little"))
        mysterysongRaw.write(bytes_per_sample.to_bytes(4, byteorder="little"))
        mysterysongRaw.write(sample_count.to_bytes(4, byteorder="little"))
        mysterysongRaw.write(mysterysongWav.readframes(sample_count))

os.remove("mysterysong.wav")

"""
mysterysong.raw format
uint32_t sample_rate
uint32_t channel_count
uint32_t bytes_per_sample
uint32_t sample_count
union { int16_t uint16_t int32_t uint32_t } samples[]
"""