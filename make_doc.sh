docker run --rm \
  -v $(pwd)/doc:/out \
  -v $(pwd)/src/mmm_api/protobuf:/protos \
  pseudomuto/protoc-gen-doc --doc_opt=markdown,docs.md:midi_internal*,enum*