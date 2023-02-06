#include "protobuf/midi.pb.h"

void protobuf_prune_events(midi::Piece *x) {
  int keep = 0;
  for (int i=0; i<x->events_size(); i++) {
    ////cout << x->events(i).should_prune() << endl;
    if (!x->events(i).should_prune()) {
      if (keep<i) {
        x->mutable_events()->SwapElements(i, keep);
      }
      ++keep;
    }
  }
  x->mutable_events()->DeleteSubrange(keep, x->events_size() - keep);
}

void protobuf_prune_tracks(midi::Piece *x) {
  int keep = 0;
  for (int i=0; i<x->tracks_size(); i++) {
    ////cout << x->tracks(i).should_prune() << endl;
    if (!x->tracks(i).should_prune()) {
      if (keep<i) {
        x->mutable_tracks()->SwapElements(i, keep);
      }
      ++keep;
    }
  }
  x->mutable_tracks()->DeleteSubrange(keep, x->tracks_size() - keep);
}

void protobuf_prune_bars(midi::Track *x) {
  int keep = 0;
  for (int i=0; i<x->bars_size(); i++) {
    ////cout << x->bars(i).should_prune() << endl;
    if (!x->bars(i).should_prune()) {
      if (keep<i) {
        x->mutable_bars()->SwapElements(i, keep);
      }
      ++keep;
    }
  }
  x->mutable_bars()->DeleteSubrange(keep, x->bars_size() - keep);
}

