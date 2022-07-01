#include "diff.hpp"
#include "xxhash64.h"

namespace diff {
    void Diff::prepareSignatures(io::FileReader &reader, bool sha) {
        signature_.clear();
        signature_.block_size =  reader.max_frame_size();

        if(sha){
            signature_.sha = calculateFileSha256(reader.file_path());
        }

        uint32_t index = 0;

        std::vector<ubyte_t> data_chunk = reader.getNextChunk();

        while (!data_chunk.empty()){
            uint32_t rolling_checksum = RHash::hashBuffer(data_chunk);
            uint64_t xx_checksum = XXHash64::hash(data_chunk.data(), data_chunk.size(),0);

            signature_.addSignature(rolling_checksum, xx_checksum, index++);

            data_chunk = reader.getNextChunk();
        }
    }

    void Diff::prepareDelta(const Signature &signature, io::FileReader &reader, bool sha) {
        delta_.clear();
        delta_.block_size = signature.block_size;
        if(sha) {
            delta_.sha = signature.sha;
        }

        RHash rhash(delta_.block_size);
        int last_found_index = -1;
        std::vector<ubyte_t> inserts;

        while(reader.rollByte()){
            rhash.roll(reader.getRolledOutByte(), reader.getLatestByte());
            inserts.push_back(reader.getLatestByte());
            auto rolling_checksum = rhash.hash();

            if(signature.signatures.contains(rolling_checksum)){

                std::vector<ubyte_t> frame = reader.getCurrentFrame();
                auto xx_checksum = XXHash64::hash(frame.data(), frame.size(),0);
                if(signature.signatures.find(rolling_checksum)->second.contains(xx_checksum)){
                    auto current_index =
                            signature.signatures.find(rolling_checksum)->second.find(xx_checksum)->second;
                    if(current_index > (last_found_index+1)){
                        delta_.deletes[last_found_index+1] = current_index-(last_found_index+1);
                    }

                    for(int i = 0; i < delta_.block_size; i++) {
                        inserts.pop_back();
                    }

                    if(!inserts.empty()) {
                        delta_.inserts[last_found_index+1] = inserts;
                        inserts.clear();
                    }
                    last_found_index = static_cast<int>(current_index);

                    continue;
                }
            }
        }

        if((last_found_index+1) < signature.signatures.size()) {
            delta_.deletes[last_found_index+1] = signature.signatures.size()-(last_found_index+1);
        }

        if(!inserts.empty()){
            delta_.inserts[last_found_index + 1] = inserts;
        }
    }

    void Diff::patchFile(const Delta &delta, io::FileReader &r_base_file,
                         io::FileWriter &w_new_file, bool check_sha, const sha256_t& checksum) {
        uint32_t index = 0;
        uint16_t chunks_to_jump;
        sha256_t sha_checksum;

        if(check_sha && checksum.empty()){
            sha_checksum = calculateFileSha256(r_base_file.file_path());
        } else {
            sha_checksum = checksum;
        }

        if(check_sha && !compareSha(delta.sha, sha_checksum)) {
            throw std::invalid_argument("Delta hash doesn't match to the base file!");
        }

        std::vector<ubyte_t> data_chunk = r_base_file.getNextChunk();

        while(!data_chunk.empty()){
            chunks_to_jump = 1;
            if(delta.inserts.contains(index)){
                w_new_file.append(delta.inserts.find(index)->second);
            }

            if(delta.deletes.contains(index)) {
                chunks_to_jump = delta.deletes.find(index)->second;
            } else {
                w_new_file.append(data_chunk);
            }

            for(auto i = 0; i < chunks_to_jump; i++, index++){
                data_chunk = r_base_file.getNextChunk();
            }
        }

        if(delta.inserts.contains(index)){
            w_new_file.append(delta.inserts.find(index)->second);
        }
    }

    sha256_t diff::Diff::calculateFileSha256(const std::string &file_path){
        std::ifstream ifs(file_path);
        char buffer[s_block_size_4k];

        std::vector<ubyte_t> sha_hash(SHA256_DIGEST_LENGTH, 0);

        SHA256_CTX ctx;
        SHA256_Init(&ctx);

        while(ifs.good()){
            ifs.read(buffer, s_block_size_4k);
            SHA256_Update(&ctx, buffer, ifs.gcount());
        }

        SHA256_Final(sha_hash.data(), &ctx);
        ifs.close();

        return sha_hash;
    }

    bool diff::Diff::compareSha(const sha256_t &hash1, const sha256_t &hash2) {
        if(hash1.size() != hash2.size()) return false;
        return std::equal(hash1.begin(), hash1.end(), hash2.begin());
    }

    void diff::Diff::generateSignatureFile(const std::string &file_path) {
        std::vector<ubyte_t> buffer = signature_.serialize();

        io::FileWriter file_writer(file_path);
        file_writer.append(buffer);

    }

    void diff::Diff::getSignatureFromFile(const std::string &file_path) {

        io::FileReader file_reader(file_path);
        std::vector<uint8_t> buffer = file_reader.getBuffer();

        if(buffer.empty()) {
            throw std::invalid_argument(std::string("File " + file_path + " empty!"));
        }

        signature_.deserialize(buffer);
    }

    void diff::Diff::generateDeltaFile(const std::string &file_path) {
        std::vector<uint8_t> buffer = delta_.serialize();

        std::vector<ubyte_t> buff = delta_.serialize();

        io::FileWriter file_writer(file_path);
        file_writer.append(buffer);
    }


    void diff::Diff::getDeltaFromFile(const std::string &file_path) {

        io::FileReader file_reader(file_path);
        std::vector<uint8_t> buffer = file_reader.getBuffer();

        if(buffer.empty()) {
            throw std::invalid_argument(std::string("File " + file_path + " is empty!"));
        }

        delta_.deserialize(buffer);
    }

    std::vector<ubyte_t> Delta::serialize() {
        std::vector<ubyte_t> buffer;
        generic_push_back(buffer, sha.size());
        std::copy(sha.begin(), sha.end(), std::back_inserter(buffer));
        generic_push_back(buffer, block_size);
        generic_push_back(buffer, inserts.size());
        for (const auto&[index_key, bytes_value]: inserts){
            generic_push_back(buffer, index_key);
            generic_push_back(buffer, bytes_value.size());
            std::copy(bytes_value.begin(), bytes_value.end(), std::back_inserter(buffer));
        }
        generic_push_back(buffer, deletes.size());
        for (const auto&[index_key, number_value]: deletes){
            generic_push_back(buffer, index_key);
            generic_push_back(buffer, number_value);
        }
        generic_push_front(buffer, buffer.size());
        return buffer;
    }

    void Delta::deserialize(std::vector<ubyte_t> buff) {
        size_t offset = 0;
        size_t buff_size = 0;
        size_t sha_size = 0;
        size_t inserts_size = 0;
        size_t deletes_size = 0;
        generic_read_var_offset(buff, offset, buff_size);
        offset += sizeof(buff_size);
        if(buff_size != (buff.size()-sizeof(buff_size))){
            throw std::invalid_argument("Invalid buffer size!");
        }

        generic_read_var_offset(buff, offset, sha_size);
        offset += sizeof(sha_size);
        sha.resize(sha_size);
        std::copy(buff.begin()+offset, buff.begin()+offset+sha_size, sha.begin());
        offset += sha_size;

        generic_read_var_offset(buff, offset, block_size);
        offset += sizeof(block_size);

        generic_read_var_offset(buff, offset, inserts_size);
        offset += sizeof(inserts_size);
        for(size_t i = 0; i < inserts_size; i++) {
            uint32_t index = 0;
            size_t bytes_size = 0;
            generic_read_var_offset(buff, offset, index);
            offset += sizeof(index);
            generic_read_var_offset(buff, offset, bytes_size);
            offset += sizeof(bytes_size);
            inserts[index] = std::vector<ubyte_t>(bytes_size);
            std::copy(buff.begin()+offset, buff.begin()+offset+bytes_size, inserts[index].begin());
            offset += bytes_size;
        }

        generic_read_var_offset(buff, offset, deletes_size);
        offset += sizeof(deletes_size);
        for(size_t i = 0; i < deletes_size; i++) {
            uint32_t index = 0;
            uint32_t chunks_num = 0;
            generic_read_var_offset(buff, offset, index);
            offset += sizeof(index);
            generic_read_var_offset(buff, offset, chunks_num);
            offset += sizeof(chunks_num);
            deletes[index] = chunks_num;
        }
    }

    void Delta::clear() {
        sha.clear();
        block_size = 0;
        inserts.clear();
        deletes.clear();
    }

    std::vector<ubyte_t> Signature::serialize() {
        std::vector<ubyte_t> buffer;
        generic_push_back(buffer, sha.size());
        std::copy(sha.begin(), sha.end(), std::back_inserter(buffer));
        generic_push_back(buffer, block_size);
        generic_push_back(buffer, signatures.size());
        for (const auto&[r_key, xx_value]: signatures){
            generic_push_back(buffer, r_key);
            generic_push_back(buffer, xx_value.size());
            for (const auto&[xx_key, index_value]: xx_value){
                generic_push_back(buffer, xx_key);
                generic_push_back(buffer, index_value);
            }
        }
        generic_push_front(buffer, buffer.size());
        return buffer;
    }

    void Signature::deserialize(std::vector<ubyte_t> buff) {
        size_t offset = 0;
        size_t buff_size = 0;
        size_t sha_size = 0;
        size_t signatures_size = 0;
        generic_read_var_offset(buff,offset, buff_size);
        offset += sizeof(buff_size);
        if(buff_size != (buff.size()-sizeof(buff_size))){
            throw std::invalid_argument("Invalid buffer size!");
        }

        generic_read_var_offset(buff,offset, sha_size);
        offset += sizeof(sha_size);
        sha.resize(sha_size);
        std::copy(buff.begin()+offset, buff.begin()+offset+sha_size, sha.begin());
        offset += sha_size;

        generic_read_var_offset(buff,offset, block_size);
        offset += sizeof(block_size);

        generic_read_var_offset(buff,offset, signatures_size);
        offset += sizeof(signatures_size);

        for(size_t i = 0; i < signatures_size; i++) {
            size_t hashes_count = 0;
            uint32_t rhash = 0;
            generic_read_var_offset(buff, offset, rhash);
            offset += sizeof(rhash);
            generic_read_var_offset(buff, offset, hashes_count);
            offset += sizeof(hashes_count);
            for(size_t j = 0; j < hashes_count; j++) {
                uint64_t xxhash = 0;
                uint32_t index = 0;
                generic_read_var_offset(buff, offset, xxhash);
                offset += sizeof(xxhash);
                generic_read_var_offset(buff, offset, index);
                offset += sizeof(index);
                signatures[rhash][xxhash] = index;
            }
        }
    }

    uint64_t Signature::countSignatures(){
        uint64_t sig_count = 0;
        for (const auto&[r_key, xx_value]: signatures){
            for (const auto&[xx_key, index_buff]: xx_value){
                sig_count++;
            }
        }
        return sig_count;
    }

    void Signature::addSignature(uint32_t rhash, uint64_t xxhash, uint32_t index) {
        signatures[rhash][xxhash] = index;
    }

    void Signature::clear() {
        sha.clear();
        block_size = 0;
        signatures.clear();
    }
}