#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include "diff.hpp"
#include "xxhash64.h"

static inline constexpr uint16_t s_block_size = 4;

class MockReader : public io::FileReader {
public:
    explicit MockReader(std::vector<diff::ubyte_t> data) {
        block_size_ = s_block_size;
        std::move(data.begin(), data.end(), std::back_inserter(data_));
        index = 0;
    }

    ~MockReader() override = default;

    std::vector<diff::ubyte_t> getNextChunk() override{
        std::vector<diff::ubyte_t> ret_buf;

        if(index >= data_.size()) return {};

        if((index + block_size_) < data_.size()) {
            std::copy(data_.begin()+index,
                      data_.begin()+index+block_size_,
                      std::back_inserter(ret_buf));
        } else {
            std::copy(data_.begin()+index,
                      data_.end(),
                      std::back_inserter(ret_buf));
        }
        index += block_size_;

        return ret_buf;
    }

    bool rollByte() override{

        if (frame_.size() == block_size_) {
            rolled_out_ = frame_.front();
            frame_.pop_front();
        }

        if(index < data_.size()){
            frame_.push_back((char)data_[index++]);
            return true;
        } else {
            return false;
        }
    }

    const uint16_t & block_size() const { return block_size_; }

private:
    uint16_t index;
    uint16_t block_size_;
    std::vector<diff::ubyte_t> data_;
};

class MockWriter : public io::FileWriter {
public:
    MockWriter() = default;

    void append(const std::vector<unsigned char> &data) override{
        std::copy(data.begin(), data.end(), std::back_inserter(data_));
    }

    const std::vector<diff::ubyte_t> & data() const { return data_; }

private:
    std::vector<diff::ubyte_t> data_;
};

struct Hashes{
    uint32_t rhash1;
    uint64_t xxhash1;
    uint32_t rhash2;
    uint64_t xxhash2;
    uint32_t rhash3;
    uint64_t xxhash3;
    uint32_t rhash4;
    uint64_t xxhash4;
    uint32_t rhash5;
    uint64_t xxhash5;
};

static std::vector<diff::ubyte_t> makeBasicBuf()
{
    return std::vector<diff::ubyte_t>{1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5};
}

static std::vector<diff::ubyte_t> makePatchBuf1()
{
    return std::vector<diff::ubyte_t>{1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0};
}

static diff::Delta makeDelta1(){
    diff::Delta delta;
    delta.inserts[5] = {0,0};
    return delta;
}

static std::vector<diff::ubyte_t> makePatchBuf2()
{
    return std::vector<diff::ubyte_t>{1,1,1,1,2,2,2,2,0,0,3,3,3,3,4,4,4,4,5,5,5,5};
}

static diff::Delta makeDelta2(){
    diff::Delta delta;
    delta.inserts[2] = {0,0};
    return delta;
}

static std::vector<diff::ubyte_t> makePatchBuf3()
{
    return std::vector<diff::ubyte_t>{0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5};
}

static diff::Delta makeDelta3(){
    diff::Delta delta;
    delta.inserts[0] = {0,0};
    return delta;
}

static std::vector<diff::ubyte_t> makePatchBuf4()
{
    return std::vector<diff::ubyte_t>{1,1,1,1,2,2,2,2,3,3,3,0,0,4,4,4,5,5,5,5};
}

static diff::Delta makeDelta4(){
    diff::Delta delta;
    delta.inserts[2] = {3,3,3,0,0,4,4,4};
    delta.deletes[2] = 2;
    return delta;
}

static std::vector<diff::ubyte_t> makePatchBuf5()
{
    return std::vector<diff::ubyte_t>{1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4};
}

static diff::Delta makeDelta5(){
    diff::Delta delta;
    delta.deletes[4] = 1;
    return delta;
}

static std::vector<diff::ubyte_t> makePatchBuf6()
{
    return std::vector<diff::ubyte_t>{2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5};
}

static diff::Delta makeDelta6(){
    diff::Delta delta;
    delta.deletes[0] = 1;
    return delta;
}

static std::vector<diff::ubyte_t> makePatchBuf7()
{
    return std::vector<diff::ubyte_t>{};
}

static diff::Delta makeDelta7(){
    diff::Delta delta;
    delta.deletes[0] = 5;
    return delta;
}

static std::vector<diff::ubyte_t> makePatchBuf8()
{
    return std::vector<diff::ubyte_t>{6,6,6,6,6};
}

static diff::Delta makeDelta8(){
    diff::Delta delta;
    delta.deletes[0] = 5;
    delta.inserts[0] = {6,6,6,6,6};
    return delta;
}

static diff::Signature prepareMockSignature(){
    diff::Signature signature;
    std::vector<diff::ubyte_t> basic_buffer = makeBasicBuf();

    uint32_t rhash1 = 655364;
    uint64_t xxhash1 = XXHash64::hash(&basic_buffer[0], s_block_size,0);
    uint32_t rhash2 = 1310728;
    uint64_t xxhash2 = XXHash64::hash(&basic_buffer[4], s_block_size,0);
    uint32_t rhash3 = 1966092;
    uint64_t xxhash3 = XXHash64::hash(&basic_buffer[8], s_block_size,0);
    uint32_t rhash4 = 2621456;
    uint64_t xxhash4 = XXHash64::hash(&basic_buffer[12], s_block_size,0);
    uint32_t rhash5 = 3276820;
    uint64_t xxhash5 = XXHash64::hash(&basic_buffer[16], s_block_size,0);

    signature.block_size = s_block_size;
    signature.addSignature(rhash1, xxhash1, 0);
    signature.addSignature(rhash2, xxhash2, 1);
    signature.addSignature(rhash3, xxhash3, 2);
    signature.addSignature(rhash4, xxhash4, 3);
    signature.addSignature(rhash5, xxhash5, 4);

    return signature;
}


static Hashes prepareBasicHashes(){
    Hashes hashes = {0};
    std::vector<diff::ubyte_t> basic_buffer = makeBasicBuf();

    hashes.rhash1 = 655364;
    hashes.xxhash1 = XXHash64::hash(&basic_buffer[0], s_block_size,0);
    hashes.rhash2 = 1310728;
    hashes.xxhash2 = XXHash64::hash(&basic_buffer[4], s_block_size,0);
    hashes.rhash3 = 1966092;
    hashes.xxhash3 = XXHash64::hash(&basic_buffer[8], s_block_size,0);
    hashes.rhash4 = 2621456;
    hashes.xxhash4 = XXHash64::hash(&basic_buffer[12], s_block_size,0);
    hashes.rhash5 = 3276820;
    hashes.xxhash5 = XXHash64::hash(&basic_buffer[16], s_block_size,0);

    return hashes;
}

TEST_CASE( "Delta serialization and deserialization", "[delta]" ) {
    diff::Delta delta;
    delta.sha = std::vector<diff::ubyte_t>(32, 1);
    delta.block_size = 4;
    delta.deletes[0] = 1;
    delta.deletes[1] = 1;
    delta.inserts[0] = std::vector<diff::ubyte_t>(12, 0xFA);
    delta.inserts[1] = std::vector<diff::ubyte_t>(12, 0xFB);
    std::vector<diff::ubyte_t> delta_buff = delta.serialize();

    diff::Delta delta2;
    delta2.deserialize(delta_buff);

    REQUIRE(delta.sha == delta2.sha);
    REQUIRE(delta.block_size == delta2.block_size);
    REQUIRE(delta.deletes.size() == delta2.deletes.size());
    REQUIRE(delta.deletes[1] == delta2.deletes[1]);
    REQUIRE(delta.inserts[1] == delta2.inserts[1]);
}

TEST_CASE( "Delta deserialization throw", "[delta]" ) {


    diff::Delta delta;

    std::vector<diff::ubyte_t> delta_buff = {0,0,0,0,0,0,0,1};

    REQUIRE_THROWS(delta.deserialize(delta_buff));
}

TEST_CASE( "Signature serialization and deserialization", "[signature]" ) {
    diff::Signature signature;
    signature.sha = std::vector<diff::ubyte_t>(32, 1);
    signature.block_size = 4;
    std::unordered_map<uint64_t, uint32_t> map;
    map[0] = 1;
    map[1] = 1;
    map[2] = 3;
    signature.signatures[0] = map;
    signature.signatures[2] = map;
    std::vector<diff::ubyte_t> signature_buff = signature.serialize();

    diff::Signature signature2;
    signature2.deserialize(signature_buff);

    REQUIRE(signature.sha == signature2.sha);
    REQUIRE(signature.block_size == signature2.block_size);
    REQUIRE(signature.signatures.size() == signature2.signatures.size());
    REQUIRE(signature.signatures[2].size() == signature2.signatures[2].size());
    REQUIRE(signature.signatures[2][0] == signature2.signatures[2][0]);
    REQUIRE(signature.signatures[0][1] == signature2.signatures[0][1]);
}

TEST_CASE( "Signature deserialization throw", "[signature]" ) {


    diff::Signature signature;

    std::vector<diff::ubyte_t> signature_buff = {0,0,0,0,0,0,0,1};

    REQUIRE_THROWS(signature.deserialize(signature_buff));
}


TEST_CASE( "Calculate block size", "[signature]" ) {

    uintmax_t file_size = 8;
    uintmax_t file_size2 = 20;
    REQUIRE(io::FileReader::calculateBlockSize(file_size) == 4);
    REQUIRE(io::FileReader::calculateBlockSize(file_size2) == 8);
}

TEST_CASE( "Generate signature", "[signature]" ) {
    diff::Diff d;
    std::vector<diff::ubyte_t> basic_buffer = makeBasicBuf();
    Hashes h = prepareBasicHashes();

    MockReader reader(basic_buffer);
    d.prepareSignatures(reader);

    diff::Signature signature = d.signature();
    REQUIRE(signature.countSignatures() == 5);
    REQUIRE(signature.signatures[h.rhash1][h.xxhash1] == 0);
    REQUIRE(signature.signatures[h.rhash2][h.xxhash2] == 1);
    REQUIRE(signature.signatures[h.rhash3][h.xxhash3] == 2);
    REQUIRE(signature.signatures[h.rhash4][h.xxhash4] == 3);
    REQUIRE(signature.signatures[h.rhash5][h.xxhash5] == 4);
}

TEST_CASE( "Generate signature not aligned", "[signature]" ) {
    diff::Diff d;
    std::vector<diff::ubyte_t> basic_buffer = makeBasicBuf();
    basic_buffer.pop_back();
    Hashes h = prepareBasicHashes();

    std::vector<diff::ubyte_t> cut_chunk = {5,5,5};
    uint32_t rhash5 = 1966095;
    uint64_t xxhash5 = XXHash64::hash(&cut_chunk[0], cut_chunk.size(),0);;

    MockReader reader(basic_buffer);
    d.prepareSignatures(reader);

    diff::Signature signature = d.signature();
    REQUIRE(signature.countSignatures() == 5);
    REQUIRE(signature.signatures[h.rhash1][h.xxhash1] == 0);
    REQUIRE(signature.signatures[h.rhash2][h.xxhash2] == 1);
    REQUIRE(signature.signatures[h.rhash3][h.xxhash3] == 2);
    REQUIRE(signature.signatures[h.rhash4][h.xxhash4] == 3);
    REQUIRE(signature.signatures[rhash5][xxhash5] == 4);
}

TEST_CASE( "Delta insert begin", "[delta]" ) {
    diff::Diff d;

    std::vector<diff::ubyte_t> original_buf = makeBasicBuf();
    std::vector<diff::ubyte_t> insertion = {0,0,0,0,0,0,0};
    std::vector<diff::ubyte_t> modified_buf;
    std::copy(insertion.begin(), insertion.end(), std::back_inserter(modified_buf));
    std::copy(original_buf.begin(), original_buf.end(), std::back_inserter(modified_buf));

    diff::Signature signature = prepareMockSignature();

    MockReader reader(modified_buf);
    d.prepareDelta(signature, reader);

    diff::Delta delta = d.delta();
    REQUIRE(delta.deletes.empty());
    REQUIRE(delta.inserts.size() == 1);
    REQUIRE(std::equal(delta.inserts.find(0)->second.begin(),
                       delta.inserts.find(0)->second.end(),
                       insertion.begin()));
}

TEST_CASE( "Delta insert end", "[delta]" ) {
    diff::Diff d;

    std::vector<diff::ubyte_t> original_buf = makeBasicBuf();
    std::vector<diff::ubyte_t> insertion = {0,0,0,0,0,0,0};
    std::vector<diff::ubyte_t> modified_buf;
    std::copy(original_buf.begin(), original_buf.end(), std::back_inserter(modified_buf));
    std::copy(insertion.begin(), insertion.end(), std::back_inserter(modified_buf));

    diff::Signature signature = prepareMockSignature();

    MockReader reader(modified_buf);
    d.prepareDelta(signature, reader);

    diff::Delta delta = d.delta();
    REQUIRE(delta.deletes.empty());
    REQUIRE(delta.inserts.size() == 1);
    REQUIRE(std::equal(delta.inserts.find(5)->second.begin(),
                       delta.inserts.find(5)->second.end(),
                       insertion.begin()));
}

TEST_CASE( "Delta insert middle 1", "[delta]" ) {
    diff::Diff d;

    uint8_t insert_position = 1;
    std::vector<diff::ubyte_t> insertion = {0,0,0,0,0,0,0};
    std::vector<diff::ubyte_t> modified_buf = makeBasicBuf();
    for (int i = 0; i < insertion.size(); i++){
        modified_buf.insert(modified_buf.begin() + (s_block_size*insert_position) + i, insertion[i]);
    }

    diff::Signature signature = prepareMockSignature();

    MockReader reader(modified_buf);
    d.prepareDelta(signature, reader);

    diff::Delta delta = d.delta();
    REQUIRE(delta.deletes.empty());
    REQUIRE(delta.inserts.size() == 1);
    REQUIRE(std::equal(delta.inserts.find(insert_position)->second.begin(),
                       delta.inserts.find(insert_position)->second.end(),
                       insertion.begin()));
}

TEST_CASE( "Delta insert middle 3", "[delta]" ) {
    diff::Diff d;

    uint8_t insert_position = 3;
    std::vector<diff::ubyte_t> insertion = {0,0,0,0,0,0,0};
    std::vector<diff::ubyte_t> modified_buf = makeBasicBuf();
    for (int i = 0; i < insertion.size(); i++){
        modified_buf.insert(modified_buf.begin() + (s_block_size*insert_position) + i, insertion[i]);
    }

    diff::Signature signature = prepareMockSignature();

    MockReader reader(modified_buf);
    d.prepareDelta(signature, reader);

    diff::Delta delta = d.delta();
    REQUIRE(delta.deletes.empty());
    REQUIRE(delta.inserts.size() == 1);
    REQUIRE(std::equal(delta.inserts.find(insert_position)->second.begin(),
                       delta.inserts.find(insert_position)->second.end(),
                       insertion.begin()));
}

TEST_CASE( "Delta delete all", "[delta]" ) {
    diff::Diff d;

    uint8_t del_position = 0;
    uint8_t del_chunks = 5;
    std::vector<diff::ubyte_t> modified_buf = {};

    diff::Signature signature = prepareMockSignature();

    MockReader reader(modified_buf);
    d.prepareDelta(signature, reader);

    diff::Delta delta = d.delta();
    REQUIRE(delta.inserts.empty());
    REQUIRE(delta.deletes.size() == 1);
    REQUIRE(delta.deletes.find(del_position)->second == del_chunks);
}

TEST_CASE( "Delta delete first", "[delta]" ) {
    diff::Diff d;

    uint8_t del_position = 0;
    uint8_t del_chunks = 1;
    std::vector<diff::ubyte_t> modified_buf = makeBasicBuf();
    for (int i = 0; i < (s_block_size * del_chunks); i++) {
        modified_buf.erase(modified_buf.begin() + (del_position * s_block_size));
    }

    diff::Signature signature = prepareMockSignature();

    MockReader reader(modified_buf);
    d.prepareDelta(signature, reader);

    diff::Delta delta = d.delta();
    REQUIRE(delta.inserts.empty());
    REQUIRE(delta.deletes.size() == 1);
    REQUIRE(delta.deletes.find(del_position)->second == del_chunks);
}

TEST_CASE( "Delta delete first two", "[delta]" ) {
    diff::Diff d;

    uint8_t del_position = 0;
    uint8_t del_chunks = 2;
    std::vector<diff::ubyte_t> modified_buf = makeBasicBuf();
    for (int i = 0; i < (s_block_size * del_chunks); i++) {
        modified_buf.erase(modified_buf.begin() + (del_position * s_block_size));
    }

    diff::Signature signature = prepareMockSignature();

    MockReader reader(modified_buf);
    d.prepareDelta(signature, reader);

    diff::Delta delta = d.delta();
    REQUIRE(delta.inserts.empty());
    REQUIRE(delta.deletes.size() == 1);
    REQUIRE(delta.deletes.find(del_position)->second == del_chunks);
}

TEST_CASE( "Delta delete last", "[delta]" ) {
    diff::Diff d;

    uint8_t del_position = 4;
    uint8_t del_chunks = 1;
    std::vector<diff::ubyte_t> modified_buf = makeBasicBuf();
    for (int i = 0; i < (s_block_size * del_chunks); i++) {
        modified_buf.erase(modified_buf.begin() + (del_position * s_block_size));
    }

    diff::Signature signature = prepareMockSignature();

    MockReader reader(modified_buf);
    d.prepareDelta(signature, reader);

    diff::Delta delta = d.delta();
    REQUIRE(delta.inserts.empty());
    REQUIRE(delta.deletes.size() == 1);
    REQUIRE(delta.deletes.find(del_position)->second == del_chunks);
}

TEST_CASE( "Delta delete middle", "[delta]" ) {
    diff::Diff d;

    uint8_t del_position = 3;
    uint8_t del_chunks = 1;
    std::vector<diff::ubyte_t> modified_buf = makeBasicBuf();
    for (int i = 0; i < (s_block_size * del_chunks); i++) {
        modified_buf.erase(modified_buf.begin() + (del_position * s_block_size));
    }

    diff::Signature signature = prepareMockSignature();

    MockReader reader(modified_buf);
    d.prepareDelta(signature, reader);

    diff::Delta delta = d.delta();
    REQUIRE(delta.inserts.empty());
    REQUIRE(delta.deletes.size() == 1);
    REQUIRE(delta.deletes.find(del_position)->second == del_chunks);
}

TEST_CASE( "Delta delete middle two in row", "[delta]" ) {
    diff::Diff d;

    uint8_t del_position = 3;
    uint8_t del_chunks = 2;
    std::vector<diff::ubyte_t> modified_buf = makeBasicBuf();
    for (int i = 0; i < (s_block_size * del_chunks); i++) {
        modified_buf.erase(modified_buf.begin() + (del_position * s_block_size));
    }

    diff::Signature signature = prepareMockSignature();

    MockReader reader(modified_buf);
    d.prepareDelta(signature, reader);

    diff::Delta delta = d.delta();
    REQUIRE(delta.inserts.empty());
    REQUIRE(delta.deletes.size() == 1);
    REQUIRE(delta.deletes.find(del_position)->second == del_chunks);
}

TEST_CASE( "Delta delete middle two with gap", "[delta]" ) {
    diff::Diff d;

    uint8_t del_position1 = 1;
    uint8_t del_position2 = 3;
    uint8_t del_chunks = 1;
    std::vector<diff::ubyte_t> modified_buf = makeBasicBuf();
    for (int i = 0; i < (s_block_size * del_chunks); i++) {
        modified_buf.erase(modified_buf.begin() + (del_position1 * s_block_size));
    }

    for (int i = 0; i < (s_block_size * del_chunks); i++) {
        modified_buf.erase(modified_buf.begin() + ((del_position2-1) * s_block_size));
    }

    diff::Signature signature = prepareMockSignature();

    MockReader reader(modified_buf);
    d.prepareDelta(signature, reader);

    diff::Delta delta = d.delta();
    REQUIRE(delta.inserts.empty());
    REQUIRE(delta.deletes.size() == 2);
    REQUIRE(delta.deletes.find(del_position1)->second == del_chunks);
    REQUIRE(delta.deletes.find(del_position2)->second == del_chunks);
}

TEST_CASE( "Patch insert back", "[patch]" ) {
    std::vector<diff::ubyte_t> basic_buff = makeBasicBuf();
    std::vector<diff::ubyte_t> final_buff = makePatchBuf1();

    diff::Delta delta = makeDelta1();

    MockWriter writer;
    MockReader reader(basic_buff);
    diff::Diff::patchFile(delta, reader, writer);


    REQUIRE(std::equal(writer.data().begin(), writer.data().end(), final_buff.begin()));
}

TEST_CASE( "Patch insert middle", "[patch]" ) {
    std::vector<diff::ubyte_t> basic_buff = makeBasicBuf();
    std::vector<diff::ubyte_t> final_buff = makePatchBuf2();

    diff::Delta delta = makeDelta2();

    MockWriter writer;
    MockReader reader(basic_buff);
    diff::Diff::patchFile(delta, reader, writer);


    REQUIRE(std::equal(writer.data().begin(), writer.data().end(), final_buff.begin()));
}

TEST_CASE( "Patch insert begin", "[patch]" ) {
    std::vector<diff::ubyte_t> basic_buff = makeBasicBuf();
    std::vector<diff::ubyte_t> final_buff = makePatchBuf3();

    diff::Delta delta = makeDelta3();

    MockWriter writer;
    MockReader reader(basic_buff);
    diff::Diff::patchFile(delta, reader, writer);


    REQUIRE(std::equal(writer.data().begin(), writer.data().end(), final_buff.begin()));
}

TEST_CASE( "Patch insert middle delete middle ", "[patch]" ) {
    std::vector<diff::ubyte_t> basic_buff = makeBasicBuf();
    std::vector<diff::ubyte_t> final_buff = makePatchBuf4();

    diff::Delta delta = makeDelta4();

    MockWriter writer;
    MockReader reader(basic_buff);
    diff::Diff::patchFile(delta, reader, writer);


    REQUIRE(std::equal(writer.data().begin(), writer.data().end(), final_buff.begin()));
}

TEST_CASE( "Patch delete end", "[patch]" ) {
    std::vector<diff::ubyte_t> basic_buff = makeBasicBuf();
    std::vector<diff::ubyte_t> final_buff = makePatchBuf5();

    diff::Delta delta = makeDelta5();

    MockWriter writer;
    MockReader reader(basic_buff);
    diff::Diff::patchFile(delta, reader, writer);


    REQUIRE(std::equal(writer.data().begin(), writer.data().end(), final_buff.begin()));
}

TEST_CASE( "Patch delete begin", "[patch]" ) {
    std::vector<diff::ubyte_t> basic_buff = makeBasicBuf();
    std::vector<diff::ubyte_t> final_buff = makePatchBuf6();

    diff::Delta delta = makeDelta6();

    MockWriter writer;
    MockReader reader(basic_buff);
    diff::Diff::patchFile(delta, reader, writer);


    REQUIRE(std::equal(writer.data().begin(), writer.data().end(), final_buff.begin()));
}

TEST_CASE( "Patch delete all", "[patch]" ) {
    std::vector<diff::ubyte_t> basic_buff = makeBasicBuf();
    std::vector<diff::ubyte_t> final_buff = makePatchBuf7();

    diff::Delta delta = makeDelta7();

    MockWriter writer;
    MockReader reader(basic_buff);
    diff::Diff::patchFile(delta, reader, writer);


    REQUIRE(std::equal(writer.data().begin(), writer.data().end(), final_buff.begin()));
}

TEST_CASE( "Patch replace all", "[patch]" ) {
    std::vector<diff::ubyte_t> basic_buff = makeBasicBuf();
    std::vector<diff::ubyte_t> final_buff = makePatchBuf8();

    diff::Delta delta = makeDelta8();

    MockWriter writer;
    MockReader reader(basic_buff);
    diff::Diff::patchFile(delta, reader, writer);


    REQUIRE(std::equal(writer.data().begin(), writer.data().end(), final_buff.begin()));
}

TEST_CASE( "Patch sha match", "[patch]" ) {
    std::vector<diff::ubyte_t> basic_buff = makeBasicBuf();

    diff::Delta delta;
    delta.sha = {1,2,3,4,5};
    diff::sha256_t sha = {1,2,3,4,5};

    MockWriter writer;
    MockReader reader(basic_buff);


    REQUIRE_NOTHROW(diff::Diff::patchFile(delta, reader, writer, true, sha));
}

TEST_CASE( "Patch sha don't match", "[patch]" ) {
    std::vector<diff::ubyte_t> basic_buff = makeBasicBuf();

    diff::Delta delta;
    delta.sha = {1,2,3,4,5};
    diff::sha256_t sha = {6,7,8,9,10};

    MockWriter writer;
    MockReader reader(basic_buff);

    REQUIRE_THROWS(diff::Diff::patchFile(delta, reader, writer, true, sha));
}






