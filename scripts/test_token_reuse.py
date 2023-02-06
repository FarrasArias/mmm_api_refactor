import mmm_api as mmm



enc = mmm.EncoderVt()

print( enc.rep.get_token_type( enc.rep.encode(mmm.MIN_PITCH, 0) ) )
print( enc.rep.get_token_type( enc.rep.encode(mmm.NOTE_ONSET, 0) ) )