#include "chess.h"
/* *INDENT-OFF* */
int scale = 100;
FILE *input_stream;
FILE *book_file;
FILE *books_file;
FILE *normal_bs_file;
FILE *computer_bs_file;
FILE *history_file;
FILE *log_file;
int presult = 0;
uint64_t total_moves;
int allow_cores = 1;
int allow_memory = 1;
int last_mate_score;
char log_filename[256];
char history_filename[256];
int number_of_solutions;
int solutions[10];
int solution_type;
char cmd_buffer[4096];
char *args[512];
char buffer[4096];
int line_length = 80;
unsigned char convert_buff[8];
int nargs;
int failhi_delta, faillo_delta;
int ponder_value;
int move_actually_played;
int ponder_move;
unsigned ponder_moves[256];
int num_ponder_moves;
int book_move;
int book_learn_eval[LEARN_INTERVAL];
int book_learn_depth[LEARN_INTERVAL];
int learn_positions_count = 0;
int learn_seekto[64];
uint64_t learn_key[64];
int learn_nmoves[64];
int book_learn_nmoves;
int book_learn_seekto;
uint64_t book_learn_key;
HASH_ENTRY *hash_table;
HPATH_ENTRY *hash_path;
PAWN_HASH_ENTRY *pawn_hash_table;
void * segments[MAX_BLOCKS + 32][2];
int nsegments = 0;
PATH last_pv;
int last_value;
int8_t directions[64][64];
int history[1024];
const uint64_t randoms[2][7][64] = {
  {
    {0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull},
    {0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0xd1fc122dd721044cull,
     0xa4159629bd0ce70eull, 0xab5da9e9ae24ad63ull, 0x32e60a2983d1c843ull,
     0x3c3cf99dabf131aaull, 0xd83283085553e1fdull, 0x180370f4abada20full,
     0xa7db417ed5cef0f6ull, 0x8940b08b9b2fc0d4ull, 0x852d84b34edc83d2ull,
     0x068d4a5f2548652full, 0x35ce432f12163d2eull, 0xc9ba66fee4843746ull,
     0xabccc0992b67af9eull, 0x217f1caa6a824b26ull, 0x4a05addc1ea2e944ull,
     0xfe2312497bf4c414ull, 0x8495248ab305ee8full, 0xcb96c4247c24e036ull,
     0xab76533d29e3c6eaull, 0xc0944c15e3c09778ull, 0x1053b4ccf6c024d6ull,
     0x8d96dda010ba133aull, 0x9f59cef04505da02ull, 0x581ae15866c42214ull,
     0x3a61654f9da998bfull, 0x47efd3ec19fb73c0ull, 0x2126b228fdb69cb5ull,
     0xbb2ff9574df0d641ull, 0x32b9d1ed571b84b8ull, 0x4f688c6727828a1full,
     0x576784e75cc9d113ull, 0x15e82e121ffd9115ull, 0xbc0156dbcef2a7deull,
     0x6365ce9628bd842eull, 0x3a898fc4ece11b80ull, 0xa6f7652af004d29full,
     0x5fa9ca7e22d71d72ull, 0x5a06111a2088be1full, 0x05aae96e3384dfe4ull,
     0xe697a8f517ab3d3full, 0x9d3c1d1302e84551ull, 0x6e0b75ab4901a6c9ull,
     0xd35c48fba6cf7eefull, 0xc8ee1ca11e20a35eull, 0x382637115387cc68ull,
     0x8d499580f4544852ull, 0x1e4a273c98576ebdull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull},
    {0x2009a159d9509cb5ull, 0x5fa8c13086419e55ull, 0xea13669343bcd4b9ull,
     0x3e47b851efbef3a4ull, 0x6caa31dec6814cc6ull, 0x992161e6ef2c2919ull,
     0x8bed069c11856a55ull, 0x5c73de210d683e98ull, 0xa7185f2616bd83c1ull,
     0xd7c04ef29cf3751eull, 0x9f46f244eaf78bcfull, 0x38f85c7bcda11a85ull,
     0x22f1e6aabfe845aaull, 0xa8aa67f9237dbb68ull, 0x90e2e582b1c84a64ull,
     0x5c685bafc31b3c9full, 0xc031fe468f6f6967ull, 0x8d75aa382752602aull,
     0x244475599eceffd5ull, 0x0d0cfcd52bb265d4ull, 0xda433164b4750dc1ull,
     0x85dfeab10d5752d6ull, 0x7225a1188a0376ebull, 0xa8be7b123a102607ull,
     0x603b914a4429875eull, 0x399be2aedab54c3bull, 0x58d916991d8a5cc4ull,
     0x35b8f864e4685bdbull, 0x46739c9baf790ac0ull, 0xd1327ef186fb215aull,
     0x140368fd88e26668ull, 0xd38e499b69fa1e25ull, 0xc96f5a7d211f3f9full,
     0x97655506a678b4bcull, 0x7f8ddfb015f07d87ull, 0xbf7bfed396840428ull,
     0xfd2f0668eb41e684ull, 0x24936e7db2759b2dull, 0x267a311ebcd14f08ull,
     0xf5a13e0e1d60b856ull, 0x69f15803d26af16aull, 0x82fac9552fa58953ull,
     0xd71ea59c19c54beaull, 0x62fdb302c66279d4ull, 0xf0c1baa7e1513de5ull,
     0x363454db33ee5ef4ull, 0xab4045843a34f2e0ull, 0x73f10c94a39100fdull,
     0x0a86593ec58517e8ull, 0xae96e568385585deull, 0x608a57252ef6f020ull,
     0x39cac56cc61f3368ull, 0x44a6a43e32a682b4ull, 0xa085261969416c01ull,
     0xbb870177729e6283ull, 0x1276bdc2fc1b7238ull, 0x77bcdc0127c4da80ull,
     0xbf16bb430520dde5ull, 0x153a63a94e385bddull, 0x95104c33771f4a98ull,
     0xb0ff0dfa8ef47265ull, 0xc48762efcfbadae4ull, 0xf70e5b4dae84cfd0ull,
     0x250cabf0859b323bull},
    {0xf71c3f6b2fd5891cull, 0x5d5675218dd4da7eull, 0xd8e7b9cd991893a1ull,
     0xe4e1af2cf1bcb046ull, 0xa762655491b63283ull, 0xd8dc072d881ba73aull,
     0xdb9cdd7d25570179ull, 0x76618a18da1cfd9aull, 0xe4ed6cee7fb62d0cull,
     0xfebc55cf05ef2fbfull, 0x9a9f5c4a59c51554ull, 0xea91c3d8f98c3ecfull,
     0x2f71c5493a5be25aull, 0x8c4d65413c6bbfe6ull, 0x9f06ed35f9a2eac6ull,
     0x178831e02b8775b2ull, 0x10a3d155d65cd6c1ull, 0x4b7a69b028a0cd53ull,
     0xd7b84fb5e0de02f0ull, 0x9e266498ec93bb5cull, 0xea0e4ad3f773090dull,
     0x4cf4b1acc505eb02ull, 0x11bea6e26eff78fdull, 0x7986363d8e9c8b02ull,
     0xff2ca02deb5af054ull, 0x3ad2351100f322d5ull, 0x24a21f70f0b14613ull,
     0x475dbafc509421a2ull, 0x9e78abd3cd79162cull, 0xe492fd5185f74274ull,
     0xc974fffbb9c5bcbdull, 0x2fdc971bd1756a88ull, 0x7229f20d9ed56e61ull,
     0x724bb3259de1f22full, 0xebdba47c4a9c567aull, 0xf93c634904fe151aull,
     0x051bcfecd0485d42ull, 0x94974a65ffaf78a4ull, 0x1e2e5e3f8b50a25dull,
     0x81b99d563b25e57aull, 0xdae46bb9fceadb75ull, 0xde35e8171244a7b5ull,
     0xf4fc75ad58cca9f2ull, 0x06396ddce8fd9c68ull, 0x9acabf60af793fb6ull,
     0x5329b615e95c5bdfull, 0xd86f5fd82254a62full, 0xf0af6c56a32479b2ull,
     0xca21baeeba56a815ull, 0x0396c3624ca42a50ull, 0xaccae9419d0f493full,
     0x6d148daab4ac1bb7ull, 0xdbd78e8efc177ba2ull, 0xcee72e2a4d16dcc6ull,
     0x4590974f7d6ec962ull, 0xd3b28408305b5764ull, 0x20c201d44df1eb89ull,
     0x0cef72d2cdef5930ull, 0xa9e4ca60ebc9a62aull, 0x8f84dab7d30110cdull,
     0x6803ad1d2c809a22ull, 0x4e3319dcbeefdd75ull, 0xb1c8fe5b88eaedbbull,
     0x3d5ceaecade87f00ull},
    {0xdb87073201334b8eull, 0x5a37d087e1ae272bull, 0x80b67cc99b9d27a1ull,
     0xd09d5f7db214f4baull, 0x3306a98928d65742ull, 0x65bc1cc4ac254147ull,
     0x92e266c8e0c61bd7ull, 0x3efe1a6dd93a7ea9ull, 0x516008fcc74b5982ull,
     0x7087c00b1219938aull, 0x54ed780d615a4d93ull, 0xf7b393019e97eb56ull,
     0x282b3182cb55067cull, 0xf74719c6e544ea8dull, 0x35629880486810e2ull,
     0xccb4db1f1f298264ull, 0x801d864f04ade5d0ull, 0xe32ae6267b0c6f71ull,
     0xc33dbe2c2656b326ull, 0x8cf005f6aced02b1ull, 0x1f4f622ce80d5b56ull,
     0x5c5166351f09065cull, 0x27ee88da0b3f17f3ull, 0x24342d232e1cc60dull,
     0xe71b0ef73358b399ull, 0x8032078133780469ull, 0xe34b1780fa310f3dull,
     0xcde826ad5c866d50ull, 0x61cbad7ae034ed02ull, 0x7ec80dc7d5443ff8ull,
     0x7552b2e6a70d2fefull, 0xb5c65d387d8a622eull, 0xcb74a3731b1404dfull,
     0x3d17820c84c52320ull, 0x012907839e217ebbull, 0xeb7f86a56d1ae07aull,
     0xf934a28184b8bc95ull, 0xe18c5514a3ffc474ull, 0x81e302d64f1e874aull,
     0x993d2d29543a0051ull, 0x4710980708601e84ull, 0xc7301ea9cc8b59beull,
     0x947c354adc6fc86full, 0xba7443084a3f3d4dull, 0x468112e49b318b2eull,
     0x636e353267f00946ull, 0xae160a34615f3ce7ull, 0xd38ce1fbfa0ae670ull,
     0x183da8dda4081ec1ull, 0xeccb6d0700512aa9ull, 0xafffc8e3770d1024ull,
     0xcc95e0c2786e2d74ull, 0x7a889c70ef7b2d7bull, 0xc1612de64cbd613full,
     0xf102a29abfbadfefull, 0x9aa9300a182763faull, 0x1bc552dcfcfd38a8ull,
     0x0a7b6521e6b8170dull, 0x2ba06c62d6b1efcdull, 0xeea0e6daafd143a8ull,
     0x9d02cbfd6f7cc234ull, 0x8aecebc23d68acb5ull, 0x1d1aa95d783617a7ull,
     0x514e9d5763edb419ull},
    {0x1e8860e4e8afcffcull, 0x107fb5302e49b653ull, 0x453ca2eb419de2c5ull,
     0xa3094e8eb1649123ull, 0xe850ac9a440dd0afull, 0xe6eb2d90ade835b6ull,
     0x75dfa9af6b2e2517ull, 0x89d272086a4586b0ull, 0x04f2d543b5fdd518ull,
     0xec4c28a9a0d6e1e7ull, 0x17258fcd264c21dbull, 0xf556bc18377a9614ull,
     0x1ed98b8d00ed78d9ull, 0x878cc5541b15235aull, 0xbdaad05ff19daba8ull,
     0x92847c097dfa71e5ull, 0x350e0dd9be966d1full, 0xa6a60d7f6f49639cull,
     0x8e070da95c30c7b1ull, 0x86cfaab004eeb6baull, 0xebaaec50d1c327f4ull,
     0xc2fe1876ab524708ull, 0xc6f6d37551ec8e3dull, 0x64d37e4c261b7ceeull,
     0x5c4238429df044d6ull, 0xffc2dcfba6020f1cull, 0x219884780eb85505ull,
     0x7b344c05cf490f3cull, 0x6357c42cdd1ece03ull, 0xe843f0447fc918ccull,
     0x3f6b9ecca16d6a9cull, 0xcadd7b5fa7f80548ull, 0x99932206ba13d48eull,
     0x2e2c3d8d923d7498ull, 0x313c5a25c7cc37ecull, 0x1ad15364afedeef6ull,
     0xfd191a29a2e31a19ull, 0x260efed6924e37c5ull, 0x59131a8c25dde4d2ull,
     0x7c21c4dec49fcb54ull, 0x7e6ce786fbf85990ull, 0xd9e19bec0755de20ull,
     0xbd6610b15f183afbull, 0x8973e87e3a7c8151ull, 0x08dc85a2ef21e267ull,
     0x8a5d053e38e38217ull, 0x362b10df55d34595ull, 0xec947b5836bd99a6ull,
     0xa86b61318e8e0669ull, 0x77fb8d13554c503bull, 0x7f2e977aecd4b847ull,
     0x1a1209b4297349a0ull, 0x246ef6a2583a91d2ull, 0xe899f0f10718bb1bull,
     0x8caf1fed80b0bf73ull, 0x6692b681b5b0cd56ull, 0x88a59e6e5279a693ull,
     0x2bca6fda2127725bull, 0xde231627953ee461ull, 0xa6a84fe6019b1505ull,
     0x3aa8d5c92821286full, 0x3fc7c4ff83788dc8ull, 0x2f1c86701e11126cull,
     0xbcf523cd44449d04ull},
    {0xb6bd2e675553abfbull, 0x0e77d5c32d27b9f1ull, 0x514eeb41d9c3d9ffull,
     0x628b101131d29234ull, 0x8ffb7c8ed0e68e19ull, 0x8ae2aad5fb090e47ull,
     0x189f977852a4f512ull, 0x95ca324d8b0efd83ull, 0x738c57e030fdae11ull,
     0x959b0a940dd59306ull, 0x65da2016543c234bull, 0x4b832a3630d9185cull,
     0x0c097ea2fdbf0d2cull, 0xfb4c8e0aa5234704ull, 0x7ac234e158f27179ull,
     0xbac5a1e34be49be8ull, 0x971437a9f55584bbull, 0x0b936a48e3a2ff38ull,
     0xce5064cedf5414ffull, 0xe843d8e0f1a1b404ull, 0x94b3ca3bb9a34c7full,
     0x2ad9eef539a002e0ull, 0x6775351b325f3972ull, 0x73a452a5d7816842ull,
     0x8f820c6a3867e2f5ull, 0x195da076e4eb0c03ull, 0xbf06d1e5880ef7e9ull,
     0x10194dde08d9bfb5ull, 0xc43bf19aac12b322ull, 0xe5574447f21ad4fbull,
     0x0022f70230a30040ull, 0xb92a66f9542cc415ull, 0x8fbcd882499ec90dull,
     0x7d7ee407c482adb5ull, 0xf284329e4afed0eaull, 0xa4a200b66d4b9a8full,
     0x9d579f70abf43ad5ull, 0x6d99c3c17a861697ull, 0xefdfea41ede917ffull,
     0x0dd85b0a545ce9ebull, 0x695f389eb81c31c4ull, 0x3e49b5c0ed676305ull,
     0x4c0792eaab653521ull, 0xae7febb40ef265f8ull, 0x735fd7bae0b300b6ull,
     0x5cd2f906d01617fdull, 0xe425e6a2194f35b8ull, 0x5e454d35558f736full,
     0xd6f7a25040a80510ull, 0x9ff8fd33a560058cull, 0xc7615283555e4d1aull,
     0x46403dffc8013c90ull, 0x4e4a44a1479e3e2eull, 0xc658ada906c5037bull,
     0x54e1529f6a6c0706ull, 0x5a929ed61f0bb3d6ull, 0x657aacb5a4eef250ull,
     0x8d75f946b56d5c44ull, 0x3852aab719722cbaull, 0xa7e416420eb9da68ull,
     0x9b7a5005d064dd92ull, 0xeaf7ce1d22dee993ull, 0x34aced1247e27fb8ull,
     0xf23478d46ca33d46ull}},
   {{0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull},
    {0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x6b90680243c4b54cull,
     0x044703d57fb1e299ull, 0x8a3c60812c08665aull, 0x32dc7de4cae88a7bull,
     0x6e3060c4bbed7feeull, 0x09a012c21457c46full, 0x713f791a21ad43caull,
     0x5a4d0ec873244cdfull, 0x59a713355c986f8full, 0xa347356dba0fbfadull,
     0x6650c485ba5b1243ull, 0x2d86094817cc64f2ull, 0x20fdc446a93f6201ull,
     0x1db9308f0d27dbdfull, 0x98f63ae9645b1111ull, 0x03415b7672cca0bdull,
     0x910f4a575767cff3ull, 0x5182927aff4f928cull, 0xdde2b660565a30dcull,
     0x3f42ec666558c5b3ull, 0xce05da712568cac7ull, 0xfd9326b46e518555ull,
     0x9334d36563994ecdull, 0x3bd434362a7b358cull, 0x14b4d64afc2171bbull,
     0x8332f03346030a46ull, 0x56300105aba021c0ull, 0x470610c2fb63b7b5ull,
     0x10c12cf0c2f837f5ull, 0x7d8af8d403969661ull, 0xa8cb40dbe096915dull,
     0xfb306c5498354397ull, 0xe25d9ee093992b8bull, 0x1f406a7e77f19817ull,
     0x06592044d4d8c7e7ull, 0x986ca3c584c84454ull, 0xcec495f755c884f0ull,
     0x3d8e0276e94a3fb3ull, 0x6dfc65f711f0e645ull, 0xf04572c328e9c0fdull,
     0x1de908cfe3f5fc3bull, 0xe1e609e5214fc6f0ull, 0x1b97e198798ccc46ull,
     0x52983c479b769f0dull, 0xe1ad316c24a875e2ull, 0x759a9b5ac9742a91ull,
     0x6bb1e5f7b2bb7e47ull, 0xbdd56e8cc40b30baull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
     0x0000000000000000ull},
    {0xe543c7cd3569a2d2ull, 0x56312cb746ff2365ull, 0x03e7298e5b86bf22ull,
     0x516e9a17bf3b9d0eull, 0xb507b363062bfe41ull, 0x7600192a061dd0e5ull,
     0x1d3fdfd7ab7ed9eeull, 0x422f696efd2a98f8ull, 0x4d3652c8c76f687cull,
     0x9f7711eae210b999ull, 0x3fe55a4a3db93342ull, 0xf9867077d2c22292ull,
     0x06938ebd5352aba1ull, 0xcfbdf8fd28e52d4aull, 0x9f5dc89b771ea0baull,
     0x76170ff911d3d955ull, 0x7e7bddf4e7d92fadull, 0x28bb97717bb86c3full,
     0xe14b10911730bfe5ull, 0x65fdc8cfbb000784ull, 0x3413b92f048583c8ull,
     0x449ccb9483380adcull, 0xa4a34f7dd630cec5ull, 0xf23142fcbf290710ull,
     0xab1645e0b73ebeb6ull, 0x9f7ac648b66b58efull, 0x81da786a8e1a3a07ull,
     0xab80b2cb27644d91ull, 0xc57b0159cd1cf77aull, 0x84425693d1e998f6ull,
     0xa086ad9f1b17e973ull, 0x6c29c7100c6ea19bull, 0x3a318a14b98d372bull,
     0xf01ba582f9382406ull, 0xa1cde875f370ae9cull, 0xb9a0ab780158707eull,
     0x8d26ff790475117eull, 0xdd33f98a55e26ad9ull, 0x549c9afccdff9451ull,
     0xfed35e6c20eabe08ull, 0xee602ea3d3b08637ull, 0xfb60f74da1a791f3ull,
     0xb25a86f4ba75b20eull, 0x5402f6bd8285b194ull, 0xb5e9f6533809151full,
     0x074c656721f3bb77ull, 0x5ccdeff9d4bbbf2dull, 0xbe262f406756394dull,
     0x105f0a553888caadull, 0xb364c84f9a7f366dull, 0x13c5a20c72a2e3ceull,
     0xb6121d22131c0104ull, 0xa4cb87927ec6cc17ull, 0x897b31544122583bull,
     0xd9bed4853c502a78ull, 0x211fee372e06645full, 0x87d38f49fd841678ull,
     0x22644edaa636120aull, 0x693c919475426cccull, 0x77b3666bd380005dull,
     0x09972e5b72ba54c1ull, 0xa6d576eb8a9adbefull, 0x151e128a1470cc43ull,
     0xba872ea0ccbcac7cull},
    {0xfaadf26b0bedcb90ull, 0x577ac5a51e5651c5ull, 0xfaa9928d90ecc574ull,
     0xf4d0746e2cd9c2cbull, 0x297b213cebe4451bull, 0xdcda00e8a1970957ull,
     0xebc24328279cf3c0ull, 0xafbc7b8a2782f71aull, 0x2b8ae9d228a44b14ull,
     0x7b91361891e74156ull, 0x7c744bd7713ebc3cull, 0xf30ff94d8ae2dbbcull,
     0x952d28d81c4cd5e5ull, 0xca1da973d1bb1a14ull, 0x0dc59391a4b26780ull,
     0x73974085e2ebdfedull, 0x6a9d797790afccadull, 0x192a2b602b123aabull,
     0x19c8a787015bc845ull, 0xad40920248a2e551ull, 0xd1c0fbdcb077da5dull,
     0xf0d3a2e4b44b9030ull, 0x4ff44c34da7b4dd0ull, 0x65ca4e0e02964227ull,
     0x1fc804c725e40440ull, 0xde8e75b6789090ffull, 0xda82400db9d07a0aull,
     0x7f850c159d730e8dull, 0x88f3cb3c8b5dbf18ull, 0xaa16f0d4828d0050ull,
     0x46a26a52fc055ad2ull, 0x9924d71d3d7359bfull, 0x73580533d9f2fb99ull,
     0x187096a491259118ull, 0x7777434ec63d90f8ull, 0x8e6e9922b252bdb1ull,
     0xb1af461d516eee12ull, 0x4fcd31f2f421c717ull, 0x65651bc93644c5e1ull,
     0x9bd1e1fa908307daull, 0x5aaee550208ae7f0ull, 0x4ec958544caaa9c3ull,
     0x298e4a3f09caee63ull, 0x29d4a3cab10a9a44ull, 0x9e6a3fde44907510ull,
     0x19cc5ee9aa4fbc78ull, 0xc01a289fe006387full, 0x3ee6737f5f934fb3ull,
     0x65fad64d232fe9eeull, 0x2a487fc2e4f569fbull, 0x67fb5df086391215ull,
     0xc1b5e63af64d55e8ull, 0xd33f764f4052ecb7ull, 0x0899d25e2b391f79ull,
     0xc70158cce44d2e70ull, 0xb53262e2308fa659ull, 0x20f7402ecd84404cull,
     0x15d1b9bb5466ad4cull, 0xfd8d825e26a8a2d1ull, 0x18d96f18f8b826d0ull,
     0xf72067659687ef21ull, 0xf08610bafd66009full, 0x1a4e1ba1fdb05563ull,
     0x1324d44c84bbef0aull},
    {0xc7613d66f20232d4ull, 0x3ede983f7b06516bull, 0x578460a649b24a39ull,
     0xd5bd4ad2cc618853ull, 0x6da1e9f12833259cull, 0x1f9ca81ea33005a0ull,
     0xae981ca25036ed10ull, 0x5d69e428228cca9aull, 0xa17023dd34f22effull,
     0xd08c493917325f88ull, 0x79e16b54bdca7e02ull, 0x3654be3ac3a26289ull,
     0xac56a76157fd0921ull, 0xc517aa54a54ed12dull, 0x4dd1b68eb1ada829ull,
     0x161037898e176e8dull, 0x98db096afb729bebull, 0x03e9d68eabc9d7feull,
     0xdf27c2fb173845dcull, 0x2aee2474d7bf2d7cull, 0xf5d6d12955e96f2dull,
     0xf734ec0a2d041943ull, 0x72999224bb580060ull, 0xccdcad5bdc7bd1a1ull,
     0xa9bca8004f1d7086ull, 0xc2c7ffb795d4e1e7ull, 0xd28da598cbfbc3caull,
     0x08126a5da13c42d9ull, 0x13e15ad5c2d3d951ull, 0x4d9c20c7eaa8703dull,
     0xab894b4e6ebca682ull, 0x53214a10ad7c784eull, 0x9906d6c9c38b591eull,
     0x476bd91a4ca0c161ull, 0x95265cbe69f01d02ull, 0x01cedc5a39e5b5c9ull,
     0xc0ef0788f08d9463ull, 0x80927db966612b20ull, 0x9630082145dd2f1eull,
     0x600f737ed7910113ull, 0xb9302c026f2a80b2ull, 0xff49c8f0afc01a91ull,
     0x39a0b9dbbc8e5d10ull, 0x4e93e5fbc38cafd7ull, 0x44d0aec75c666288ull,
     0x5ea41ef6508a97efull, 0x6ce58a3a4917dffdull, 0x57b84b8764d3da3dull,
     0xedf320cbe664658cull, 0x3bea7e195d96172aull, 0x72abd9c2b876c142ull,
     0xaf2ce404a46dae6eull, 0x4f050de918e728b2ull, 0xcb458cba72881a48ull,
     0xdadcfcb48f1c02a8ull, 0x1167bf4ccb9f1a34ull, 0x2f791f047047bd5dull,
     0xcd60c5789e26b8b6ull, 0x1a620288f4a12b4bull, 0x1b43a4a5b7f5244dull,
     0x3a93e22c7f6b3c55ull, 0xa5da9b1ba501f044ull, 0xac918f8b4d9d1e00ull,
     0x9fba7867a63c8ac8ull},
    {0xb5663909d9d6303aull, 0x1122bbd8f31a9801ull, 0xb26dfdc7254c0ac9ull,
     0x80923ffe2bae8db2ull, 0xdf3939952977e95full, 0x89ef22889d7081a6ull,
     0xddeeb25f2e41e526ull, 0x77aa072d06890a48ull, 0x8b1e7a1bc43beb1cull,
     0xeef27b1803a60f3aull, 0x116569c8ba82a83aull, 0x861eddf3fdf4d64full,
     0x12fc5033c7c95105ull, 0xc9997d1a2f05161aull, 0x56dbac3597f7e48bull,
     0x997968b0dadb71b6ull, 0x51c153dd787cf748ull, 0x190b253068b60e60ull,
     0xa98dcc93091ee1daull, 0xe7f1c48a17f0c994ull, 0x294532d1f3b50a20ull,
     0xe393a663d106d4b2ull, 0xc95a8e15d4e4aac2ull, 0x058a1a4819387af4ull,
     0xb7fe4077025d3331ull, 0x17369b1ad4dfb135ull, 0xfd1836fd44897416ull,
     0x734f98ea7a95ea1dull, 0x44dbebde2de33051ull, 0x3e572ff979d8ca38ull,
     0xd53c5a45bb8cdfa1ull, 0x1169ade998830992ull, 0xc6b5b477d2ee43b5ull,
     0x11d58b895af5f73aull, 0xcfe3985db2d35d21ull, 0xc9c6056490e28221ull,
     0xbba44fb18d7bad4cull, 0xd1b94354c3d22c4dull, 0xfb0d0d4c55eedddeull,
     0x3b18e9ef00b4c810ull, 0x73e101f840df8084ull, 0xd64f148443724752ull,
     0xb017cbfb12688bd6ull, 0x89e6a53131fc7242ull, 0x2f6bc2d724af9792ull,
     0x1af46d6374011c6aull, 0xe7d461f15c6129b4ull, 0xbd7b0f8478d446abull,
     0x8cab2463b6c0e01dull, 0xa69dee16a765d2b0ull, 0x144588401f496bf3ull,
     0x3d761d20063ee258ull, 0x48c0b32df8ddc0fbull, 0x1ad8889a5aa8e26cull,
     0x2aadb6180f80c2d3ull, 0xb7c9d582a54c0b2cull, 0xa9448d0f698b8370ull,
     0xd6814d04b2584c63ull, 0x80576b83319d83f9ull, 0x906953398a3df494ull,
     0xe7a11b9d7d769494ull, 0x59714b37b93b5e39ull, 0xa5280a61ef2d0450ull,
     0xcaf1ca6cd004e7bcull},
    {0x0c92df7ab48210acull, 0x70c766782c6225c0ull, 0x2b627e280a8dd01full,
     0xf8a95606e064f51bull, 0xecb6d461dd6c8568ull, 0xe8e9d8da88a760e7ull,
     0xb253ddc5e1b54ff4ull, 0xb518eacb142499c5ull, 0x5ba23807ace2576bull,
     0xb5d274ebd42fcb9cull, 0x1dfc510cd7016641ull, 0x81b2aa898d7ff740ull,
     0xb3f8b22a412d350full, 0x9010d26bd30013d6ull, 0x31a160801ed3585full,
     0xf18717011ddb123bull, 0x475fb6cd262d3895ull, 0xf86a9bada37fe981ull,
     0x579fe8f10c63060cull, 0x1ea46e3bcbee6f47ull, 0x0dfa846a5626e47aull,
     0xe76ff8e4aab118a0ull, 0xa83a45a05758d1c4ull, 0xff293f1d1de94a79ull,
     0x6d34106328ce50acull, 0x7f3dd6bb2c715f0dull, 0x01a6483cc3fb62c0ull,
     0xa60927ade8ccdca7ull, 0x73c1dc8d32c0180full, 0x02f86bcc14474ff9ull,
     0x3804de0c37c58434ull, 0xfc10f3a3c497c54dull, 0x96a1e55142ddb8dbull,
     0xc92548d8939af17aull, 0xbc5edf6509acaf89ull, 0xac0b9688d3023544ull,
     0x4163dbff847088b3ull, 0x563f3cfce243d3f8ull, 0x8f263f7ce1f7b3ccull,
     0xe7365cbc6a7f8730ull, 0x46c1f063e6b8ca39ull, 0x21cbc42ba4582264ull,
     0x55dff0476966e4b4ull, 0x223e2c38b61edc4eull, 0x3a21ced28a3d6fa8ull,
     0xd5884ee48c058d27ull, 0x884d4eac614c987aull, 0x02327b02f1a6a37full,
     0xec14f49f926b0d1dull, 0xad980ec0c9b3ccdcull, 0xad0f89f58a31f96cull,
     0x1004ad6869a8c64dull, 0x73334f7053ecda9cull, 0xe5c726eb2395f91full,
     0x3eacccee6b0d2cf0ull, 0x54fe44475c2803fbull, 0x4e691ecbfdff4c35ull,
     0x0d4dd3188efdf8f3ull, 0x4ce513f999517686ull, 0x451033ddedb79722ull,
     0x7fafe619290cf26eull, 0xb744be4992d915b4ull, 0x8011bf394690d8d1ull,
     0x9475361b15b45cd5ull},
   },
};
const uint64_t castle_random[2][2] = {
  {0x557723689550b69bull, 0xa92c541efa336c6cull},
  {0xc7bedab779d5361bull, 0x3bb70e80435e60b7ull}
};
const uint64_t enpassant_random[65] = {
  0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
  0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
  0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
  0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
  0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
  0x0000000000000000ull, 0xf2c1412f44c13c98ull, 0x76b4b7ccb0c9bd1eull,
  0x0303f047ef3166cdull, 0xcf4da3850ff5c35aull, 0x0bb57340632ec140ull,
  0x189156c368616498ull, 0x71b862b8cede277dull, 0x26e0433817e6d7d7ull,
  0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
  0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
  0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
  0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
  0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
  0x0000000000000000ull, 0x1f2af0448165ab3aull, 0x51f0d423276d44dbull,
  0x12d51a6ba742f661ull, 0x8fa3e91c53630e1full, 0x16573a4eb7f48c08ull,
  0xe1c1e4bc9690e409ull, 0x5f2bf4422dde33bbull, 0xcd4cefba64f407a1ull,
  0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
  0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
  0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
  0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
  0x0000000000000000ull, 0x0000000000000000ull, 0x0000000000000000ull,
  0x0000000000000000ull, 0x0000000000000000ull
};
uint64_t pawn_attacks[2][64];
uint64_t knight_attacks[64];
uint64_t rook_attacks[64];
uint64_t bishop_attacks[64];
uint64_t king_attacks[64];
uint64_t intervening[64][64];
uint64_t clear_mask[65];
uint64_t set_mask[65];
uint64_t file_mask[8];
uint64_t rank_mask[8];
const uint64_t OO[2] =  {0x6000000000000000ull, 0x0000000000000060ull};
const uint64_t OOO[2] = {0x0E00000000000000ull, 0x000000000000000Eull};
const uint64_t mask_efgh =        0xf0f0f0f0f0f0f0f0ull;
const uint64_t mask_fgh =         0xe0e0e0e0e0e0e0e0ull;
const uint64_t mask_abc =         0x0707070707070707ull;
const uint64_t mask_abcd =        0x0f0f0f0f0f0f0f0full;
const uint64_t mask_advance_2_w = 0x0000000000ff0000ull;
const uint64_t mask_advance_2_b = 0x0000ff0000000000ull;
const uint64_t mask_left_edge =   0xfefefefefefefefeull;
const uint64_t mask_right_edge =  0x7f7f7f7f7f7f7f7full;
const uint64_t mask_not_edge =    0x007e7e7e7e7e7e00ull;
const uint64_t mask_rook_files =  0x8181818181818181ull;
const uint64_t dark_squares =     0xaa55aa55aa55aa55ull;
const uint64_t not_rook_pawns =   0x007e7e7e7e7e7e00ull;
uint64_t plus1dir[65];
uint64_t plus7dir[65];
uint64_t plus8dir[65];
uint64_t plus9dir[65];
uint64_t minus1dir[65];
uint64_t minus7dir[65];
uint64_t minus8dir[65];
uint64_t minus9dir[65];
uint64_t mask_eptest[64];
POSITION display;
#if !defined(INLINEASM)
unsigned char msb[65536];
unsigned char lsb[65536];
#endif
unsigned char msb_8bit[256];
unsigned char lsb_8bit[256];
unsigned char pop_cnt_8bit[256];
uint64_t mask_pawn_connected[2][64];
uint64_t mask_pawn_isolated[64];
uint64_t mask_passed[2][64];
uint64_t mask_pattacks[2][64];
uint64_t pawn_race[2][2][64];
BOOK_POSITION book_buffer[BOOK_CLUSTER_SIZE];
BOOK_POSITION book_buffer_char[BOOK_CLUSTER_SIZE];
const int OOsqs[2][3] =  {{E8, F8, G8}, {E1, F1, G1}};
const int OOOsqs[2][3] = {{E8, D8, C8}, {E1, D1, C1}};
const int OOfrom[2] = {E8, E1};
const int OOto[2] =   {G8, G1};
const int OOOto[2] =  {C8, C1};
#define VERSION      "25.2"
char version[8] = {VERSION};
PLAYING_MODE mode = normal_mode;
int batch_mode = 0;                  /* no asynch reads */
int swindle_mode = 1;                /* try to swindle */
int call_flag = 0;
int crafty_rating = 2500;
int opponent_rating = 2500;
int last_search_value = 0;
int FP_margin[16] = {  0, 100, 150, 200,  250,  300,  400,  500,
                     600, 700, 800, 900, 1000, 1100, 1200, 1300 };
int FP_depth = 7;
int LMP[16];
int LMP_depth = 16;
int LMP_base = 3;
double LMP_scale = 1.9;
int pgn_suggested_percent = 0;
char pgn_event[128] = {"?"};
char pgn_site[128] = {"?"};
char pgn_date[128] = {"????.??.??"};
char pgn_round[128] = {"?"};
char pgn_white[128] = {"unknown"};
char pgn_white_elo[128] = {""};
char pgn_black[128] = {"Crafty " VERSION};
char pgn_black_elo[128] = {""};
char pgn_result[128] = {"*"};
char *B_list[128];
char *AK_list[128];
char *GM_list[128];
char *IM_list[128];
char *SP_list[128];
char *SP_opening_filename[128];
char *SP_personality_filename[128];
int output_format = 0;
#if defined(SYZYGY)
int EGTBlimit = 0;
int EGTB_use = 0;
int EGTB_draw = 0;
int EGTB_depth = 0;
size_t EGTB_cache_size = 4096 * 4096;
void *EGTB_cache = (void *) 0;
int EGTB_setup = 0;
#endif
int xboard = 0;
int xboard_done = 0;
int pong = 0;
int early_exit = 99;
char book_path[128] = {BOOKDIR};
char log_path[128] = {LOGDIR};
char tb_path[128] = {TBDIR};
char rc_path[128] = {RCDIR};
int initialized = 0;
int kibitz = 0;
int post = 0;
int log_id = 1;
int game_wtm = 1;
int last_opponent_move = 0;
int check_depth = 1;                  /* extend checks one ply                */
int null_depth = 3;                   /* R=3 + (next line)                    */
int null_divisor = 6;                 /* R = null_depth + depth / 6           */
int LMR_rdepth = 1;                   /* leave 1 full ply after reductions    */
int LMR_min= 1;                       /* minimum reduction 1 ply              */
int LMR_max= 15;                      /* maximum reduction 15 plies           */
double LMR_db = 1.8;                  /* depth is 1.8x as important as        */
double LMR_mb = 1.0;                  /* moves searched in the formula.       */
double LMR_s = 2.0;                   /* smaller numbers increase reductions. */
uint8_t LMR[32][64];
int rep_index;
int search_depth = 0;
uint64_t search_nodes = 0;
uint64_t temp_search_nodes = 0;
int search_move = 0;
int predicted = 0;
int time_used = 0;
int time_used_opponent = 0;
int analyze_mode = 0;
int annotate_mode = 0;
int input_status = 0;
int resign = 9;
int resign_counter = 0;
int resign_count = 5;
int draw_counter = 0;
int draw_count = 5;
int draw_offer_pending = 0;
int draw_offered = 0;
int offer_draws = 1;
int dynamic_draw_score = 1;
int adaptive_hash = 0;
size_t adaptive_hash_min = 0;
size_t adaptive_hash_max = 0;
size_t adaptive_hashp_min = 0;
size_t adaptive_hashp_max = 0;
int time_limit = 100;
int force = 0;
char initial_position[80] = {""};
char ponder_text[512] = {""};
char book_hint[512] = {""};
int over = 0;
int usage_level = 0;
char audible_alarm = 0x07;
char speech = 0;
int book_accept_mask = ~03;
int book_reject_mask = 3;
int book_random = 1;
float book_weight_learn = 1.0;
float book_weight_freq = 1.0;
float book_weight_eval = 0.1;
int book_search_trigger = 20;
int learn = 1;
int learning = 100;
int learn_value = 0;
int abort_search;      /*  1 = abort / print stats, 2 = abort no print stats */
int iteration;
int root_wtm = 1;
int last_root_value;
ROOT_MOVE root_moves[256];
int n_root_moves;
int difficulty;
int absolute_time_limit;
int search_time_limit;
int burp;
int quit = 0;
unsigned opponent_start_time, opponent_end_time;
unsigned program_start_time, program_end_time;
unsigned start_time, end_time;
TREE *block[MAX_BLOCKS + 1];
THREAD thread[CPUS];
#if (CPUS > 1)
lock_t lock_smp, lock_io;
#if defined(UNIX)
  pthread_attr_t attributes;
#endif
#endif
unsigned int hardware_processors;
unsigned int smp_max_threads = 0;
unsigned int smp_split_group = 8;       /* max threads per group              */
unsigned int smp_split_at_root = 1;     /* enable split at root               */
unsigned int smp_min_split_depth = 5;   /* don't split within 5 plies of tips */
unsigned int smp_gratuitous_depth = 10; /* gratuitous splits if depth > 10    */
unsigned int smp_gratuitous_limit = 6;  /* max gratuitous splits / thread     */
int smp_nice = 1;                       /* terminate idle threads             */
int smp_affinity = 0;                   /* anything >= 0 is enabled           */
int smp_affinity_increment = 1;         /* if physical cores are zero - N-1 
                                           use 1, if physical cores are even
                                           numbers and hyperthreaded cores are
                                           odd use 2.  For IBM POWER 8 use 8  */
int smp_numa = 0;                       /* disables NUMA mode by default      */
                                        /* enable if you really have NUMA     */
/*
      This is the autotune configuration section.  Each line represents one
      smp search parameter that can be tuned.  The first three values are the
      min, max and increment that autotune will use for this paramenter.  The
      fourth parameter is a string autotune will display to explain what it is
      currently tuning.  The fifth parameter is the command used to change this
      parameter (this is added to the .craftyrc/crafty.rc file after tuning is
      completed.  The last parameter is a pointer to the variable that must be
      changed to adjust the parameter while running the autotune tests.
*/
int autotune_params = 4;
struct autotune tune[16] = {
  { 4, 20, 4, "max thread group", "smpgroup", &smp_split_group},
  { 4,  8, 1, "min split depth", "smpmin", &smp_min_split_depth},
  {10, 16, 2, "min gratuitous split depth", "smpgsd", &smp_gratuitous_depth},
  { 1,  8, 2, "gratuitous split limit", "smpgsl", &smp_gratuitous_limit},
};
unsigned parallel_splits;
unsigned parallel_splits_wasted;
unsigned parallel_aborts;
unsigned parallel_joins;
unsigned busy_percent = 0;
uint64_t game_max_blocks = 0;
volatile int smp_split = 0;
volatile int smp_threads = 0;
volatile int initialized_threads = 0;
int crafty_is_white = 0;
unsigned nodes_between_time_checks = 1000000;
unsigned nodes_per_second = 1000000;
int next_time_check = 100000;
int transposition_age = 0;
int thinking = 0;
int pondering = 0;
int puzzling = 0;
int booking = 0;
/************************************************************
 *                                                          *
 *    1 -> display move/time/results/etc.                   *
 *    2 -> display PV.                                      *
 *    4 -> display fail high / fail low moves               *
 *    8 -> display search statistics.                       *
 *   16 -> display root moves as they are searched.         *
 *   32 -> display general informational messages.          *
 *   64 -> display ply-1 move list / flags after each       *
 *           iteration.                                     *
 *  128 -> display root moves and scores before search      *
 *           begins.                                        *
 * 2048 -> error messages (can not be disabled).            *
 *                                                          *
 ************************************************************
 */
int display_options = 1 | 2 | 8 | 16 | 32 | 2048;
unsigned noise_level = 100;
int noise_block = 0;
int tc_moves = 60;
int tc_time = 180000;
int tc_time_remaining[2] = {180000, 180000};
int tc_moves_remaining[2] = {60, 60};
int tc_secondary_moves = 30;
int tc_secondary_time = 90000;
int tc_increment = 0;
int tc_sudden_death = 0;
int tc_safety_margin = 0;
int draw_score[2] = {0, 0};
char kibitz_text[4096];
int kibitz_depth;
int move_number = 1;
int moves_out_of_book = 0;
int first_nonbook_factor = 0;
int first_nonbook_span = 0;
#if defined(SKILL)
int skill = 100;
#endif
int show_book = 0;
int book_selection_width = 5;
int ponder = 1;
int trace_level = 0;
/*  for the following 6 lines, each pair should have */
/*  the same numeric value (the size value).         */
size_t hash_table_size = 1ull << 20;
uint64_t hash_mask = ((1ull << 20) - 1) & ~3;
size_t hash_path_size = 1ull << 16;
uint64_t hash_path_mask = ((1ull << 16) - 1) & ~15;
size_t pawn_hash_table_size = 1ull << 18;
uint64_t pawn_hash_mask = (1ull << 18) - 1;
uint64_t mask_clear_entry = 0xff9ffffffffe0000ull;
int abs_draw_score = 0;
int accept_draws = 1;
const char translate[13] =
    {'k', 'q', 'r', 'b', 'n', 'p', 0, 'P', 'N', 'B', 'R', 'Q', 'K'};
const uint64_t magic_bishop[64] = {
  0x0002020202020200ull, 0x0002020202020000ull, 0x0004010202000000ull,
  0x0004040080000000ull, 0x0001104000000000ull, 0x0000821040000000ull,
  0x0000410410400000ull, 0x0000104104104000ull, 0x0000040404040400ull,
  0x0000020202020200ull, 0x0000040102020000ull, 0x0000040400800000ull,
  0x0000011040000000ull, 0x0000008210400000ull, 0x0000004104104000ull,
  0x0000002082082000ull, 0x0004000808080800ull, 0x0002000404040400ull,
  0x0001000202020200ull, 0x0000800802004000ull, 0x0000800400A00000ull,
  0x0000200100884000ull, 0x0000400082082000ull, 0x0000200041041000ull,
  0x0002080010101000ull, 0x0001040008080800ull, 0x0000208004010400ull,
  0x0000404004010200ull, 0x0000840000802000ull, 0x0000404002011000ull,
  0x0000808001041000ull, 0x0000404000820800ull, 0x0001041000202000ull,
  0x0000820800101000ull, 0x0000104400080800ull, 0x0000020080080080ull,
  0x0000404040040100ull, 0x0000808100020100ull, 0x0001010100020800ull,
  0x0000808080010400ull, 0x0000820820004000ull, 0x0000410410002000ull,
  0x0000082088001000ull, 0x0000002011000800ull, 0x0000080100400400ull,
  0x0001010101000200ull, 0x0002020202000400ull, 0x0001010101000200ull,
  0x0000410410400000ull, 0x0000208208200000ull, 0x0000002084100000ull,
  0x0000000020880000ull, 0x0000001002020000ull, 0x0000040408020000ull,
  0x0004040404040000ull, 0x0002020202020000ull, 0x0000104104104000ull,
  0x0000002082082000ull, 0x0000000020841000ull, 0x0000000000208800ull,
  0x0000000010020200ull, 0x0000000404080200ull, 0x0000040404040400ull,
  0x0002020202020200ull
};
const uint64_t magic_bishop_mask[64] = {
  0x0040201008040200ull, 0x0000402010080400ull, 0x0000004020100A00ull,
  0x0000000040221400ull, 0x0000000002442800ull, 0x0000000204085000ull,
  0x0000020408102000ull, 0x0002040810204000ull, 0x0020100804020000ull,
  0x0040201008040000ull, 0x00004020100A0000ull, 0x0000004022140000ull,
  0x0000000244280000ull, 0x0000020408500000ull, 0x0002040810200000ull,
  0x0004081020400000ull, 0x0010080402000200ull, 0x0020100804000400ull,
  0x004020100A000A00ull, 0x0000402214001400ull, 0x0000024428002800ull,
  0x0002040850005000ull, 0x0004081020002000ull, 0x0008102040004000ull,
  0x0008040200020400ull, 0x0010080400040800ull, 0x0020100A000A1000ull,
  0x0040221400142200ull, 0x0002442800284400ull, 0x0004085000500800ull,
  0x0008102000201000ull, 0x0010204000402000ull, 0x0004020002040800ull,
  0x0008040004081000ull, 0x00100A000A102000ull, 0x0022140014224000ull,
  0x0044280028440200ull, 0x0008500050080400ull, 0x0010200020100800ull,
  0x0020400040201000ull, 0x0002000204081000ull, 0x0004000408102000ull,
  0x000A000A10204000ull, 0x0014001422400000ull, 0x0028002844020000ull,
  0x0050005008040200ull, 0x0020002010080400ull, 0x0040004020100800ull,
  0x0000020408102000ull, 0x0000040810204000ull, 0x00000A1020400000ull,
  0x0000142240000000ull, 0x0000284402000000ull, 0x0000500804020000ull,
  0x0000201008040200ull, 0x0000402010080400ull, 0x0002040810204000ull,
  0x0004081020400000ull, 0x000A102040000000ull, 0x0014224000000000ull,
  0x0028440200000000ull, 0x0050080402000000ull, 0x0020100804020000ull,
  0x0040201008040200ull
};
const unsigned magic_bishop_shift[64] = {
  58, 59, 59, 59, 59, 59, 59, 58,
  59, 59, 59, 59, 59, 59, 59, 59,
  59, 59, 57, 57, 57, 57, 59, 59,
  59, 59, 57, 55, 55, 57, 59, 59,
  59, 59, 57, 55, 55, 57, 59, 59,
  59, 59, 57, 57, 57, 57, 59, 59,
  59, 59, 59, 59, 59, 59, 59, 59,
  58, 59, 59, 59, 59, 59, 59, 58
};
uint64_t magic_bishop_table[5248];
uint64_t *magic_bishop_indices[64] = {
  magic_bishop_table + 4992, magic_bishop_table + 2624,
  magic_bishop_table + 256, magic_bishop_table + 896,
  magic_bishop_table + 1280, magic_bishop_table + 1664,
  magic_bishop_table + 4800, magic_bishop_table + 5120,
  magic_bishop_table + 2560, magic_bishop_table + 2656,
  magic_bishop_table + 288, magic_bishop_table + 928,
  magic_bishop_table + 1312, magic_bishop_table + 1696,
  magic_bishop_table + 4832, magic_bishop_table + 4928,
  magic_bishop_table + 0, magic_bishop_table + 128,
  magic_bishop_table + 320, magic_bishop_table + 960,
  magic_bishop_table + 1344, magic_bishop_table + 1728,
  magic_bishop_table + 2304, magic_bishop_table + 2432,
  magic_bishop_table + 32, magic_bishop_table + 160,
  magic_bishop_table + 448, magic_bishop_table + 2752,
  magic_bishop_table + 3776, magic_bishop_table + 1856,
  magic_bishop_table + 2336, magic_bishop_table + 2464,
  magic_bishop_table + 64, magic_bishop_table + 192,
  magic_bishop_table + 576, magic_bishop_table + 3264,
  magic_bishop_table + 4288, magic_bishop_table + 1984,
  magic_bishop_table + 2368, magic_bishop_table + 2496,
  magic_bishop_table + 96, magic_bishop_table + 224,
  magic_bishop_table + 704, magic_bishop_table + 1088,
  magic_bishop_table + 1472, magic_bishop_table + 2112,
  magic_bishop_table + 2400, magic_bishop_table + 2528,
  magic_bishop_table + 2592, magic_bishop_table + 2688,
  magic_bishop_table + 832, magic_bishop_table + 1216,
  magic_bishop_table + 1600, magic_bishop_table + 2240,
  magic_bishop_table + 4864, magic_bishop_table + 4960,
  magic_bishop_table + 5056, magic_bishop_table + 2720,
  magic_bishop_table + 864, magic_bishop_table + 1248,
  magic_bishop_table + 1632, magic_bishop_table + 2272,
  magic_bishop_table + 4896, magic_bishop_table + 5184
};
int16_t magic_bishop_mobility_table[5248];
int16_t *magic_bishop_mobility_indices[64] = {
  magic_bishop_mobility_table + 4992, magic_bishop_mobility_table + 2624,
  magic_bishop_mobility_table + 256, magic_bishop_mobility_table + 896,
  magic_bishop_mobility_table + 1280, magic_bishop_mobility_table + 1664,
  magic_bishop_mobility_table + 4800, magic_bishop_mobility_table + 5120,
  magic_bishop_mobility_table + 2560, magic_bishop_mobility_table + 2656,
  magic_bishop_mobility_table + 288, magic_bishop_mobility_table + 928,
  magic_bishop_mobility_table + 1312, magic_bishop_mobility_table + 1696,
  magic_bishop_mobility_table + 4832, magic_bishop_mobility_table + 4928,
  magic_bishop_mobility_table + 0, magic_bishop_mobility_table + 128,
  magic_bishop_mobility_table + 320, magic_bishop_mobility_table + 960,
  magic_bishop_mobility_table + 1344, magic_bishop_mobility_table + 1728,
  magic_bishop_mobility_table + 2304, magic_bishop_mobility_table + 2432,
  magic_bishop_mobility_table + 32, magic_bishop_mobility_table + 160,
  magic_bishop_mobility_table + 448, magic_bishop_mobility_table + 2752,
  magic_bishop_mobility_table + 3776, magic_bishop_mobility_table + 1856,
  magic_bishop_mobility_table + 2336, magic_bishop_mobility_table + 2464,
  magic_bishop_mobility_table + 64, magic_bishop_mobility_table + 192,
  magic_bishop_mobility_table + 576, magic_bishop_mobility_table + 3264,
  magic_bishop_mobility_table + 4288, magic_bishop_mobility_table + 1984,
  magic_bishop_mobility_table + 2368, magic_bishop_mobility_table + 2496,
  magic_bishop_mobility_table + 96, magic_bishop_mobility_table + 224,
  magic_bishop_mobility_table + 704, magic_bishop_mobility_table + 1088,
  magic_bishop_mobility_table + 1472, magic_bishop_mobility_table + 2112,
  magic_bishop_mobility_table + 2400, magic_bishop_mobility_table + 2528,
  magic_bishop_mobility_table + 2592, magic_bishop_mobility_table + 2688,
  magic_bishop_mobility_table + 832, magic_bishop_mobility_table + 1216,
  magic_bishop_mobility_table + 1600, magic_bishop_mobility_table + 2240,
  magic_bishop_mobility_table + 4864, magic_bishop_mobility_table + 4960,
  magic_bishop_mobility_table + 5056, magic_bishop_mobility_table + 2720,
  magic_bishop_mobility_table + 864, magic_bishop_mobility_table + 1248,
  magic_bishop_mobility_table + 1632, magic_bishop_mobility_table + 2272,
  magic_bishop_mobility_table + 4896, magic_bishop_mobility_table + 5184
};
uint64_t magic_rook_table[102400];
uint64_t *magic_rook_indices[64] = {
  magic_rook_table + 86016, magic_rook_table + 73728,
  magic_rook_table + 36864, magic_rook_table + 43008,
  magic_rook_table + 47104, magic_rook_table + 51200,
  magic_rook_table + 77824, magic_rook_table + 94208,
  magic_rook_table + 69632, magic_rook_table + 32768,
  magic_rook_table + 38912, magic_rook_table + 10240,
  magic_rook_table + 14336, magic_rook_table + 53248,
  magic_rook_table + 57344, magic_rook_table + 81920,
  magic_rook_table + 24576, magic_rook_table + 33792,
  magic_rook_table + 6144, magic_rook_table + 11264,
  magic_rook_table + 15360, magic_rook_table + 18432,
  magic_rook_table + 58368, magic_rook_table + 61440,
  magic_rook_table + 26624, magic_rook_table + 4096,
  magic_rook_table + 7168, magic_rook_table + 0,
  magic_rook_table + 2048, magic_rook_table + 19456,
  magic_rook_table + 22528, magic_rook_table + 63488,
  magic_rook_table + 28672, magic_rook_table + 5120,
  magic_rook_table + 8192, magic_rook_table + 1024,
  magic_rook_table + 3072, magic_rook_table + 20480,
  magic_rook_table + 23552, magic_rook_table + 65536,
  magic_rook_table + 30720, magic_rook_table + 34816,
  magic_rook_table + 9216, magic_rook_table + 12288,
  magic_rook_table + 16384, magic_rook_table + 21504,
  magic_rook_table + 59392, magic_rook_table + 67584,
  magic_rook_table + 71680, magic_rook_table + 35840,
  magic_rook_table + 39936, magic_rook_table + 13312,
  magic_rook_table + 17408, magic_rook_table + 54272,
  magic_rook_table + 60416, magic_rook_table + 83968,
  magic_rook_table + 90112, magic_rook_table + 75776,
  magic_rook_table + 40960, magic_rook_table + 45056,
  magic_rook_table + 49152, magic_rook_table + 55296,
  magic_rook_table + 79872, magic_rook_table + 98304
};
int16_t magic_rook_mobility_table[102400];
int16_t *magic_rook_mobility_indices[64] = {
  magic_rook_mobility_table + 86016, magic_rook_mobility_table + 73728,
  magic_rook_mobility_table + 36864, magic_rook_mobility_table + 43008,
  magic_rook_mobility_table + 47104, magic_rook_mobility_table + 51200,
  magic_rook_mobility_table + 77824, magic_rook_mobility_table + 94208,
  magic_rook_mobility_table + 69632, magic_rook_mobility_table + 32768,
  magic_rook_mobility_table + 38912, magic_rook_mobility_table + 10240,
  magic_rook_mobility_table + 14336, magic_rook_mobility_table + 53248,
  magic_rook_mobility_table + 57344, magic_rook_mobility_table + 81920,
  magic_rook_mobility_table + 24576, magic_rook_mobility_table + 33792,
  magic_rook_mobility_table + 6144, magic_rook_mobility_table + 11264,
  magic_rook_mobility_table + 15360, magic_rook_mobility_table + 18432,
  magic_rook_mobility_table + 58368, magic_rook_mobility_table + 61440,
  magic_rook_mobility_table + 26624, magic_rook_mobility_table + 4096,
  magic_rook_mobility_table + 7168, magic_rook_mobility_table + 0,
  magic_rook_mobility_table + 2048, magic_rook_mobility_table + 19456,
  magic_rook_mobility_table + 22528, magic_rook_mobility_table + 63488,
  magic_rook_mobility_table + 28672, magic_rook_mobility_table + 5120,
  magic_rook_mobility_table + 8192, magic_rook_mobility_table + 1024,
  magic_rook_mobility_table + 3072, magic_rook_mobility_table + 20480,
  magic_rook_mobility_table + 23552, magic_rook_mobility_table + 65536,
  magic_rook_mobility_table + 30720, magic_rook_mobility_table + 34816,
  magic_rook_mobility_table + 9216, magic_rook_mobility_table + 12288,
  magic_rook_mobility_table + 16384, magic_rook_mobility_table + 21504,
  magic_rook_mobility_table + 59392, magic_rook_mobility_table + 67584,
  magic_rook_mobility_table + 71680, magic_rook_mobility_table + 35840,
  magic_rook_mobility_table + 39936, magic_rook_mobility_table + 13312,
  magic_rook_mobility_table + 17408, magic_rook_mobility_table + 54272,
  magic_rook_mobility_table + 60416, magic_rook_mobility_table + 83968,
  magic_rook_mobility_table + 90112, magic_rook_mobility_table + 75776,
  magic_rook_mobility_table + 40960, magic_rook_mobility_table + 45056,
  magic_rook_mobility_table + 49152, magic_rook_mobility_table + 55296,
  magic_rook_mobility_table + 79872, magic_rook_mobility_table + 98304
};
const uint64_t magic_rook[64] = {
  0x0080001020400080ull, 0x0040001000200040ull, 0x0080081000200080ull,
  0x0080040800100080ull, 0x0080020400080080ull, 0x0080010200040080ull,
  0x0080008001000200ull, 0x0080002040800100ull, 0x0000800020400080ull,
  0x0000400020005000ull, 0x0000801000200080ull, 0x0000800800100080ull,
  0x0000800400080080ull, 0x0000800200040080ull, 0x0000800100020080ull,
  0x0000800040800100ull, 0x0000208000400080ull, 0x0000404000201000ull,
  0x0000808010002000ull, 0x0000808008001000ull, 0x0000808004000800ull,
  0x0000808002000400ull, 0x0000010100020004ull, 0x0000020000408104ull,
  0x0000208080004000ull, 0x0000200040005000ull, 0x0000100080200080ull,
  0x0000080080100080ull, 0x0000040080080080ull, 0x0000020080040080ull,
  0x0000010080800200ull, 0x0000800080004100ull, 0x0000204000800080ull,
  0x0000200040401000ull, 0x0000100080802000ull, 0x0000080080801000ull,
  0x0000040080800800ull, 0x0000020080800400ull, 0x0000020001010004ull,
  0x0000800040800100ull, 0x0000204000808000ull, 0x0000200040008080ull,
  0x0000100020008080ull, 0x0000080010008080ull, 0x0000040008008080ull,
  0x0000020004008080ull, 0x0000010002008080ull, 0x0000004081020004ull,
  0x0000204000800080ull, 0x0000200040008080ull, 0x0000100020008080ull,
  0x0000080010008080ull, 0x0000040008008080ull, 0x0000020004008080ull,
  0x0000800100020080ull, 0x0000800041000080ull, 0x00FFFCDDFCED714Aull,
  0x007FFCDDFCED714Aull, 0x003FFFCDFFD88096ull, 0x0000040810002101ull,
  0x0001000204080011ull, 0x0001000204000801ull, 0x0001000082000401ull,
  0x0001FFFAABFAD1A2ull
};
const uint64_t magic_rook_mask[64] = {
  0x000101010101017Eull, 0x000202020202027Cull, 0x000404040404047Aull,
  0x0008080808080876ull, 0x001010101010106Eull, 0x002020202020205Eull,
  0x004040404040403Eull, 0x008080808080807Eull, 0x0001010101017E00ull,
  0x0002020202027C00ull, 0x0004040404047A00ull, 0x0008080808087600ull,
  0x0010101010106E00ull, 0x0020202020205E00ull, 0x0040404040403E00ull,
  0x0080808080807E00ull, 0x00010101017E0100ull, 0x00020202027C0200ull,
  0x00040404047A0400ull, 0x0008080808760800ull, 0x00101010106E1000ull,
  0x00202020205E2000ull, 0x00404040403E4000ull, 0x00808080807E8000ull,
  0x000101017E010100ull, 0x000202027C020200ull, 0x000404047A040400ull,
  0x0008080876080800ull, 0x001010106E101000ull, 0x002020205E202000ull,
  0x004040403E404000ull, 0x008080807E808000ull, 0x0001017E01010100ull,
  0x0002027C02020200ull, 0x0004047A04040400ull, 0x0008087608080800ull,
  0x0010106E10101000ull, 0x0020205E20202000ull, 0x0040403E40404000ull,
  0x0080807E80808000ull, 0x00017E0101010100ull, 0x00027C0202020200ull,
  0x00047A0404040400ull, 0x0008760808080800ull, 0x00106E1010101000ull,
  0x00205E2020202000ull, 0x00403E4040404000ull, 0x00807E8080808000ull,
  0x007E010101010100ull, 0x007C020202020200ull, 0x007A040404040400ull,
  0x0076080808080800ull, 0x006E101010101000ull, 0x005E202020202000ull,
  0x003E404040404000ull, 0x007E808080808000ull, 0x7E01010101010100ull,
  0x7C02020202020200ull, 0x7A04040404040400ull, 0x7608080808080800ull,
  0x6E10101010101000ull, 0x5E20202020202000ull, 0x3E40404040404000ull,
  0x7E80808080808000ull
};
const unsigned magic_rook_shift[64] = {
  52, 53, 53, 53, 53, 53, 53, 52,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 54, 54, 54, 54, 53,
  53, 54, 54, 53, 53, 53, 53, 53
};
const uint64_t mobility_mask_b[4] = {
  0xFF818181818181FFull, 0x007E424242427E00ull,
  0x00003C24243C0000ull, 0x0000001818000000ull
};
const uint64_t mobility_mask_r[4] = {
  0x8181818181818181ull, 0x4242424242424242ull,
  0x2424242424242424ull, 0x1818181818181818ull
};
/*
  values use to deal with white/black independently
 */
const int rankflip[2][8] = {
  {RANK8, RANK7, RANK6, RANK5, RANK4, RANK3, RANK2, RANK1},
  {RANK1, RANK2, RANK3, RANK4, RANK5, RANK6, RANK7, RANK8}
};
const int sqflip[2][64] = {
 {A8, B8, C8, D8, E8, F8, G8, H8,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A4, B4, C4, D4, E4, F4, G4, H4,   /* black */
  A3, B3, C3, D3, E3, F3, G3, H3,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A1, B1, C1, D1, E1, F1, G1, H1},

 {A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,   /* white */
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8}
};
const int rank1[2] = {RANK8, RANK1};
const int rank2[2] = {RANK7, RANK2};
const int rank3[2] = {RANK6, RANK3};
const int rank4[2] = {RANK5, RANK4};
const int rank5[2] = {RANK4, RANK5};
const int rank6[2] = {RANK3, RANK6};
const int rank7[2] = {RANK2, RANK7};
const int rank8[2] = {RANK1, RANK8};
const int epdir[2] = {8, -8};
const int csq[2] = {C8, C1};
const int dsq[2] = {D8, D1};
const int esq[2] = {E8, E1};
const int fsq[2] = {F8, F1};
const int gsq[2] = {G8, G1};
uint64_t distance_ring[64][8];
const int sign[2] = {-1, 1};
const int epsq[2] = {+8, -8};
const int rook_A[2] = {A8, A1};
const int rook_D[2] = {D8, D1};
const int rook_F[2] = {F8, F1};
const int rook_G[2] = {G8, G1};
const int rook_H[2] = {H8, H1};
const int pawnadv1[2] = {+8, -8};
const int pawnadv2[2] = {+16, -16};
const int capleft[2] = {+9, -7};
const int capright[2] = {+7, -9};
const char empty_sqs[9] = {0, '1', '2', '3', '4', '5', '6', '7', '8'};
const int pcval[7] = {0, 100, 300, 300, 500, 900, 9900};
const int p_vals[7] = {0, 1, 3, 3, 5, 9, 99};
const int MVV_LVA[7][7] = {
    {0,  5<<21,  4<<21,  4<<21,  3<<21,  2<<21,  1<<21},
    {0, 10<<21,  9<<21,  9<<21,  8<<21,  7<<21,  6<<21},
    {0, 15<<21, 14<<21, 14<<21, 13<<21, 12<<21, 11<<21},
    {0, 15<<21, 14<<21, 14<<21, 13<<21, 12<<21, 11<<21},
    {0, 20<<21, 19<<21, 19<<21, 18<<21, 17<<21, 16<<21},
    {0, 25<<21, 24<<21, 24<<21, 23<<21, 22<<21, 21<<21},
    {0, 30<<21, 29<<21, 29<<21, 28<<21, 27<<21, 26<<21}};
const int pieces[2][7] = {
  {0, -1, -2, -3, -4, -5, -6},
  {0, +1, +2, +3, +4, +5, +6}
};
const int lower_n = 16;
const int lower_b = 10;
const int mobility_score_n[4] = {1, 2, 3, 4};
const int mobility_score_b[4] = {1, 2, 3, 4};
const int mobility_score_r[4] = {1, 2, 3, 4};
const int mob_curve_r[48] = {
  -27,-23,-21,-19,-15,-10, -9, -8,
   -7, -6, -5, -4, -3, -2, -1,  0,
    1,  2,  3,  4,  5,  6,  7,  8,
    9, 10, 11, 12, 13, 14, 15, 16,
   17, 18, 19, 20, 21, 22, 23, 24,
   25, 26, 27, 28, 29, 30, 31, 32
};
int piece_values[2][7] = {
  {0, -PAWN_VALUE, -KNIGHT_VALUE, -BISHOP_VALUE,
      -ROOK_VALUE, -QUEEN_VALUE,    -KING_VALUE},
  {0,  PAWN_VALUE,  KNIGHT_VALUE,  BISHOP_VALUE,
       ROOK_VALUE,  QUEEN_VALUE,     KING_VALUE}
};
/*
   First term is a character string explaining what the eval
   term is used for.

  Second term is the "type" of the value.
     0 = heading entry (no values, just a category name to display).
     1 = scalar value.
     2 = array[mg]
     3 = floating point value

   Third term is the "size" of the scoring term, where 0 is a
     scalar value, otherwise it is the actual number of elements in
     an array of values.

   Fourth term is a pointer to the data value(s).
*/
struct personality_term personality_packet[256] = {
  {"search options                          ", 0, 0, NULL},        /*  0 */
  {"check extension                         ", 1, 0, &check_depth},
  {"null-move reduction                     ", 1, 0, &null_depth},
  {"null-move adaptive divisor              ", 1, 0, &null_divisor},
  {"LMR min distance to frontier            ", 1, 0, &LMR_rdepth},
  {"LMR min reduction                       ", 1, 0, &LMR_min},
  {"LMR max reduction                       ", 1, 0, &LMR_max},
  {"LMR formula depth bias                  ", 3, 0, &LMR_db},
  {"LMR formula moves searched bias         ", 3, 0, &LMR_mb},
  {"LMR scale factor                        ", 3, 0, &LMR_s},
  {"search options (continued)              ", 0, 0, NULL},        /* 10 */
  {NULL, 0, 0, NULL},
};
/* *INDENT-ON* */
