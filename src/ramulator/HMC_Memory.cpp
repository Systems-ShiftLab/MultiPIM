
#include "HMC_Memory.h"

using namespace ramulator;

namespace ramulator
{

// int Memory<HMC, Controller>::get_cpu_slid(int cub, long addr)
// {
//     int slid;
//     if (topology == DRAGONFLY)
//     {
//         if (stacks == 4)
//         {
//             if (cub == 0)
//                 slid = 0;
//             else if (cub == 1)
//                 slid = 4;
//             else if (cub == 2)
//                 slid = 8;
//             else if (cub == 3)
//                 slid = 12;
//             else
//                 assert(0 && "error cub id");
//         }
//         else if (stacks == 8)
//         {
//             int cub_group = cub / 2;
//             if (cub_group == 0)
//                 slid = 0;
//             else if (cub_group == 1)
//                 slid = 12;
//             else if (cub_group == 2)
//                 slid = 16;
//             else if (cub_group == 3)
//                 slid = 28;
//             else
//                 assert(0 && "error cub id");
//         }
//         else if (stacks == 16)
//         {
//             int cub_group = cub / 4;
//             if (cub_group == 0)
//                 slid = 0;
//             else if (cub_group == 1)
//                 slid = 16;
//             else if (cub_group == 2)
//                 slid = 32;
//             else if (cub_group == 3)
//                 slid = 48;
//             else
//                 assert(0 && "error cub id");
//         }
//         else
//         {
//             assert(0);
//         }
//     }
//     else if (topology == MESH)
//     {
//         //"Mesh"
//         if (stacks == 16)
//         {
//             int cub_pair = cub / 2;
//             if (cub_pair == 0 || cub_pair == 2)
//                 slid = 0;
//             else if (cub_pair == 1 || cub_pair == 3)
//                 slid = 12;
//             else if (cub_pair == 4 || cub_pair == 6)
//                 slid = 48;
//             else if (cub_pair == 5 || cub_pair == 7)
//                 slid = 60;
//             else
//                 assert(0 && "error cub id");
//         }
//         else if (stacks == 8)
//         {
//             int cub_pair = cub / 2;
//             if (cub_pair == 0)
//                 slid = 0;
//             else if (cub_pair == 1)
//                 slid = 12;
//             else if (cub_pair == 2)
//                 slid = 16;
//             else if (cub_pair == 3)
//                 slid = 28;
//             else
//                 assert(0 && "error cub id");
//         }
//         else if (stacks == 4)
//         {
//             if (cub == 0)
//                 slid = 0;
//             else if (cub == 1)
//                 slid = 4;
//             else if (cub == 2)
//                 slid = 8;
//             else if (cub == 3)
//                 slid = 12;
//             else
//                 assert(0 && "error cub id");
//         }
//         else
//         {
//             assert(0);
//         }
//     }
//     else
//     {
//         //Single
//         slid = addr % spec->source_links;
//     }

//     return slid;
// }

void Memory<HMC,Controller>::connect_hmcs_Mesh()
{
    if (stacks == 16)
    {
        logic_layers[0]->get_link_by_num(1)->set_other_side_link(logic_layers[1]->get_link_by_num(4));
        logic_layers[0]->get_link_by_num(2)->set_other_side_link(logic_layers[4]->get_link_by_num(16));

        logic_layers[1]->get_link_by_num(4)->set_other_side_link(logic_layers[0]->get_link_by_num(1));
        logic_layers[1]->get_link_by_num(6)->set_other_side_link(logic_layers[2]->get_link_by_num(8));
        logic_layers[1]->get_link_by_num(7)->set_other_side_link(logic_layers[5]->get_link_by_num(21));

        logic_layers[2]->get_link_by_num(8)->set_other_side_link(logic_layers[1]->get_link_by_num(6));
        logic_layers[2]->get_link_by_num(10)->set_other_side_link(logic_layers[3]->get_link_by_num(15));
        logic_layers[2]->get_link_by_num(11)->set_other_side_link(logic_layers[6]->get_link_by_num(25));

        logic_layers[3]->get_link_by_num(14)->set_other_side_link(logic_layers[7]->get_link_by_num(29));
        logic_layers[3]->get_link_by_num(15)->set_other_side_link(logic_layers[2]->get_link_by_num(10));

        logic_layers[4]->get_link_by_num(16)->set_other_side_link(logic_layers[0]->get_link_by_num(2));
        logic_layers[4]->get_link_by_num(17)->set_other_side_link(logic_layers[5]->get_link_by_num(20));
        logic_layers[4]->get_link_by_num(18)->set_other_side_link(logic_layers[8]->get_link_by_num(32));

        logic_layers[5]->get_link_by_num(20)->set_other_side_link(logic_layers[4]->get_link_by_num(17));
        logic_layers[5]->get_link_by_num(21)->set_other_side_link(logic_layers[1]->get_link_by_num(7));
        logic_layers[5]->get_link_by_num(22)->set_other_side_link(logic_layers[6]->get_link_by_num(24));
        logic_layers[5]->get_link_by_num(23)->set_other_side_link(logic_layers[9]->get_link_by_num(37));

        logic_layers[6]->get_link_by_num(24)->set_other_side_link(logic_layers[5]->get_link_by_num(22));
        logic_layers[6]->get_link_by_num(25)->set_other_side_link(logic_layers[2]->get_link_by_num(11));
        logic_layers[6]->get_link_by_num(26)->set_other_side_link(logic_layers[7]->get_link_by_num(28));
        logic_layers[6]->get_link_by_num(27)->set_other_side_link(logic_layers[10]->get_link_by_num(41));

        logic_layers[7]->get_link_by_num(28)->set_other_side_link(logic_layers[6]->get_link_by_num(26));
        logic_layers[7]->get_link_by_num(29)->set_other_side_link(logic_layers[3]->get_link_by_num(14));
        logic_layers[7]->get_link_by_num(31)->set_other_side_link(logic_layers[11]->get_link_by_num(45));

        logic_layers[8]->get_link_by_num(32)->set_other_side_link(logic_layers[4]->get_link_by_num(18));
        logic_layers[8]->get_link_by_num(33)->set_other_side_link(logic_layers[9]->get_link_by_num(36));
        logic_layers[8]->get_link_by_num(34)->set_other_side_link(logic_layers[12]->get_link_by_num(49));

        logic_layers[9]->get_link_by_num(36)->set_other_side_link(logic_layers[8]->get_link_by_num(33));
        logic_layers[9]->get_link_by_num(37)->set_other_side_link(logic_layers[5]->get_link_by_num(23));
        logic_layers[9]->get_link_by_num(38)->set_other_side_link(logic_layers[10]->get_link_by_num(40));
        logic_layers[9]->get_link_by_num(39)->set_other_side_link(logic_layers[13]->get_link_by_num(53));

        logic_layers[10]->get_link_by_num(40)->set_other_side_link(logic_layers[9]->get_link_by_num(38));
        logic_layers[10]->get_link_by_num(41)->set_other_side_link(logic_layers[6]->get_link_by_num(27));
        logic_layers[10]->get_link_by_num(42)->set_other_side_link(logic_layers[11]->get_link_by_num(44));
        logic_layers[10]->get_link_by_num(43)->set_other_side_link(logic_layers[14]->get_link_by_num(57));

        logic_layers[11]->get_link_by_num(44)->set_other_side_link(logic_layers[10]->get_link_by_num(42));
        logic_layers[11]->get_link_by_num(45)->set_other_side_link(logic_layers[7]->get_link_by_num(31));
        logic_layers[11]->get_link_by_num(47)->set_other_side_link(logic_layers[15]->get_link_by_num(63));

        logic_layers[12]->get_link_by_num(49)->set_other_side_link(logic_layers[8]->get_link_by_num(34));
        logic_layers[12]->get_link_by_num(50)->set_other_side_link(logic_layers[13]->get_link_by_num(52));

        logic_layers[13]->get_link_by_num(52)->set_other_side_link(logic_layers[12]->get_link_by_num(50));
        logic_layers[13]->get_link_by_num(53)->set_other_side_link(logic_layers[9]->get_link_by_num(39));
        logic_layers[13]->get_link_by_num(54)->set_other_side_link(logic_layers[14]->get_link_by_num(56));

        logic_layers[14]->get_link_by_num(56)->set_other_side_link(logic_layers[13]->get_link_by_num(54));
        logic_layers[14]->get_link_by_num(57)->set_other_side_link(logic_layers[10]->get_link_by_num(43));
        logic_layers[14]->get_link_by_num(58)->set_other_side_link(logic_layers[15]->get_link_by_num(62));

        logic_layers[15]->get_link_by_num(62)->set_other_side_link(logic_layers[14]->get_link_by_num(58));
        logic_layers[15]->get_link_by_num(63)->set_other_side_link(logic_layers[11]->get_link_by_num(47));
    }else if(stacks == 8){
        logic_layers[0]->get_link_by_num(1)->set_other_side_link(logic_layers[1]->get_link_by_num(4));
        logic_layers[0]->get_link_by_num(2)->set_other_side_link(logic_layers[4]->get_link_by_num(17));

        logic_layers[1]->get_link_by_num(4)->set_other_side_link(logic_layers[0]->get_link_by_num(1));
        logic_layers[1]->get_link_by_num(6)->set_other_side_link(logic_layers[2]->get_link_by_num(8));
        logic_layers[1]->get_link_by_num(7)->set_other_side_link(logic_layers[5]->get_link_by_num(21));

        logic_layers[2]->get_link_by_num(8)->set_other_side_link(logic_layers[1]->get_link_by_num(6));
        logic_layers[2]->get_link_by_num(10)->set_other_side_link(logic_layers[3]->get_link_by_num(15));
        logic_layers[2]->get_link_by_num(11)->set_other_side_link(logic_layers[6]->get_link_by_num(25));

        logic_layers[3]->get_link_by_num(14)->set_other_side_link(logic_layers[7]->get_link_by_num(31));
        logic_layers[3]->get_link_by_num(15)->set_other_side_link(logic_layers[2]->get_link_by_num(10));

        logic_layers[4]->get_link_by_num(17)->set_other_side_link(logic_layers[0]->get_link_by_num(2));
        logic_layers[4]->get_link_by_num(18)->set_other_side_link(logic_layers[5]->get_link_by_num(20));

        logic_layers[5]->get_link_by_num(20)->set_other_side_link(logic_layers[4]->get_link_by_num(18));
        logic_layers[5]->get_link_by_num(21)->set_other_side_link(logic_layers[1]->get_link_by_num(7));
        logic_layers[5]->get_link_by_num(22)->set_other_side_link(logic_layers[6]->get_link_by_num(24));

        logic_layers[6]->get_link_by_num(24)->set_other_side_link(logic_layers[5]->get_link_by_num(22));
        logic_layers[6]->get_link_by_num(25)->set_other_side_link(logic_layers[2]->get_link_by_num(11));
        logic_layers[6]->get_link_by_num(26)->set_other_side_link(logic_layers[7]->get_link_by_num(30));

        logic_layers[7]->get_link_by_num(30)->set_other_side_link(logic_layers[6]->get_link_by_num(26));
        logic_layers[7]->get_link_by_num(31)->set_other_side_link(logic_layers[3]->get_link_by_num(14));
    } else if(stacks == 4){
        logic_layers[0]->get_link_by_num(1)->set_other_side_link(logic_layers[1]->get_link_by_num(7));
        logic_layers[0]->get_link_by_num(2)->set_other_side_link(logic_layers[2]->get_link_by_num(9));

        logic_layers[1]->get_link_by_num(6)->set_other_side_link(logic_layers[3]->get_link_by_num(15));
        logic_layers[1]->get_link_by_num(7)->set_other_side_link(logic_layers[0]->get_link_by_num(1));

        logic_layers[2]->get_link_by_num(9)->set_other_side_link(logic_layers[0]->get_link_by_num(2));
        logic_layers[2]->get_link_by_num(10)->set_other_side_link(logic_layers[3]->get_link_by_num(14));

        logic_layers[3]->get_link_by_num(14)->set_other_side_link(logic_layers[2]->get_link_by_num(10));
        logic_layers[3]->get_link_by_num(15)->set_other_side_link(logic_layers[1]->get_link_by_num(6));

    }
    else
    {
        assert(0 && "Only support 16 stacks for Mesh topology!");
    }
}

void Memory<HMC, Controller>::connect_hmcs_Dragonfly()
{
    if (stacks == 4){
        logic_layers[0]->get_link_by_num(1)->set_other_side_link(logic_layers[1]->get_link_by_num(7));
        logic_layers[0]->get_link_by_num(2)->set_other_side_link(logic_layers[3]->get_link_by_num(14));
        logic_layers[0]->get_link_by_num(3)->set_other_side_link(logic_layers[2]->get_link_by_num(9));

        logic_layers[1]->get_link_by_num(5)->set_other_side_link(logic_layers[3]->get_link_by_num(15));
        logic_layers[1]->get_link_by_num(6)->set_other_side_link(logic_layers[2]->get_link_by_num(10));
        logic_layers[1]->get_link_by_num(7)->set_other_side_link(logic_layers[0]->get_link_by_num(1));

        logic_layers[2]->get_link_by_num(9)->set_other_side_link(logic_layers[0]->get_link_by_num(3));
        logic_layers[2]->get_link_by_num(10)->set_other_side_link(logic_layers[1]->get_link_by_num(6));
        logic_layers[2]->get_link_by_num(11)->set_other_side_link(logic_layers[3]->get_link_by_num(13));

        logic_layers[3]->get_link_by_num(13)->set_other_side_link(logic_layers[2]->get_link_by_num(11));
        logic_layers[3]->get_link_by_num(14)->set_other_side_link(logic_layers[0]->get_link_by_num(2));
        logic_layers[3]->get_link_by_num(15)->set_other_side_link(logic_layers[1]->get_link_by_num(5));
    }
    else if (stacks == 8)
    {

        logic_layers[0]->get_link_by_num(1)->set_other_side_link(logic_layers[1]->get_link_by_num(4));
        logic_layers[0]->get_link_by_num(2)->set_other_side_link(logic_layers[2]->get_link_by_num(11));
        logic_layers[0]->get_link_by_num(3)->set_other_side_link(logic_layers[3]->get_link_by_num(13));

        logic_layers[1]->get_link_by_num(4)->set_other_side_link(logic_layers[0]->get_link_by_num(1));
        logic_layers[1]->get_link_by_num(5)->set_other_side_link(logic_layers[5]->get_link_by_num(21));
        logic_layers[1]->get_link_by_num(6)->set_other_side_link(logic_layers[2]->get_link_by_num(8));
        logic_layers[1]->get_link_by_num(7)->set_other_side_link(logic_layers[3]->get_link_by_num(14));

        logic_layers[2]->get_link_by_num(8)->set_other_side_link(logic_layers[1]->get_link_by_num(6));
        logic_layers[2]->get_link_by_num(9)->set_other_side_link(logic_layers[6]->get_link_by_num(25));
        logic_layers[2]->get_link_by_num(10)->set_other_side_link(logic_layers[3]->get_link_by_num(15));
        logic_layers[2]->get_link_by_num(11)->set_other_side_link(logic_layers[0]->get_link_by_num(2));

        logic_layers[3]->get_link_by_num(13)->set_other_side_link(logic_layers[0]->get_link_by_num(3));
        logic_layers[3]->get_link_by_num(14)->set_other_side_link(logic_layers[1]->get_link_by_num(7));
        logic_layers[3]->get_link_by_num(15)->set_other_side_link(logic_layers[2]->get_link_by_num(10));

        logic_layers[4]->get_link_by_num(17)->set_other_side_link(logic_layers[5]->get_link_by_num(20));
        logic_layers[4]->get_link_by_num(18)->set_other_side_link(logic_layers[6]->get_link_by_num(27));
        logic_layers[4]->get_link_by_num(19)->set_other_side_link(logic_layers[7]->get_link_by_num(29));

        logic_layers[5]->get_link_by_num(20)->set_other_side_link(logic_layers[4]->get_link_by_num(17));
        logic_layers[5]->get_link_by_num(21)->set_other_side_link(logic_layers[1]->get_link_by_num(5));
        logic_layers[5]->get_link_by_num(22)->set_other_side_link(logic_layers[6]->get_link_by_num(24));
        logic_layers[5]->get_link_by_num(23)->set_other_side_link(logic_layers[7]->get_link_by_num(30));

        logic_layers[6]->get_link_by_num(24)->set_other_side_link(logic_layers[5]->get_link_by_num(22));
        logic_layers[6]->get_link_by_num(25)->set_other_side_link(logic_layers[2]->get_link_by_num(9));
        logic_layers[6]->get_link_by_num(26)->set_other_side_link(logic_layers[7]->get_link_by_num(31));
        logic_layers[6]->get_link_by_num(27)->set_other_side_link(logic_layers[4]->get_link_by_num(18));

        logic_layers[7]->get_link_by_num(29)->set_other_side_link(logic_layers[4]->get_link_by_num(19));
        logic_layers[7]->get_link_by_num(30)->set_other_side_link(logic_layers[5]->get_link_by_num(23));
        logic_layers[7]->get_link_by_num(31)->set_other_side_link(logic_layers[6]->get_link_by_num(26));
    }
    else if (stacks == 16)
    {

        logic_layers[0]->get_link_by_num(1)->set_other_side_link(logic_layers[1]->get_link_by_num(4));
        logic_layers[0]->get_link_by_num(2)->set_other_side_link(logic_layers[2]->get_link_by_num(11));
        logic_layers[0]->get_link_by_num(3)->set_other_side_link(logic_layers[3]->get_link_by_num(13));

        logic_layers[1]->get_link_by_num(4)->set_other_side_link(logic_layers[0]->get_link_by_num(1));
        logic_layers[1]->get_link_by_num(5)->set_other_side_link(logic_layers[5]->get_link_by_num(21));
        logic_layers[1]->get_link_by_num(6)->set_other_side_link(logic_layers[2]->get_link_by_num(8));
        logic_layers[1]->get_link_by_num(7)->set_other_side_link(logic_layers[3]->get_link_by_num(14));

        logic_layers[2]->get_link_by_num(8)->set_other_side_link(logic_layers[1]->get_link_by_num(6));
        logic_layers[2]->get_link_by_num(9)->set_other_side_link(logic_layers[14]->get_link_by_num(57));
        logic_layers[2]->get_link_by_num(10)->set_other_side_link(logic_layers[3]->get_link_by_num(15));
        logic_layers[2]->get_link_by_num(11)->set_other_side_link(logic_layers[0]->get_link_by_num(2));

        logic_layers[3]->get_link_by_num(12)->set_other_side_link(logic_layers[9]->get_link_by_num(37));
        logic_layers[3]->get_link_by_num(13)->set_other_side_link(logic_layers[0]->get_link_by_num(3));
        logic_layers[3]->get_link_by_num(14)->set_other_side_link(logic_layers[1]->get_link_by_num(7));
        logic_layers[3]->get_link_by_num(15)->set_other_side_link(logic_layers[2]->get_link_by_num(10));

        logic_layers[4]->get_link_by_num(17)->set_other_side_link(logic_layers[5]->get_link_by_num(20));
        logic_layers[4]->get_link_by_num(18)->set_other_side_link(logic_layers[6]->get_link_by_num(27));
        logic_layers[4]->get_link_by_num(19)->set_other_side_link(logic_layers[7]->get_link_by_num(29));

        logic_layers[5]->get_link_by_num(20)->set_other_side_link(logic_layers[4]->get_link_by_num(17));
        logic_layers[5]->get_link_by_num(21)->set_other_side_link(logic_layers[1]->get_link_by_num(5));
        logic_layers[5]->get_link_by_num(22)->set_other_side_link(logic_layers[6]->get_link_by_num(24));
        logic_layers[5]->get_link_by_num(23)->set_other_side_link(logic_layers[7]->get_link_by_num(30));

        logic_layers[6]->get_link_by_num(24)->set_other_side_link(logic_layers[5]->get_link_by_num(22));
        logic_layers[6]->get_link_by_num(25)->set_other_side_link(logic_layers[10]->get_link_by_num(41));
        logic_layers[6]->get_link_by_num(26)->set_other_side_link(logic_layers[7]->get_link_by_num(31));
        logic_layers[6]->get_link_by_num(27)->set_other_side_link(logic_layers[4]->get_link_by_num(18));

        logic_layers[7]->get_link_by_num(28)->set_other_side_link(logic_layers[13]->get_link_by_num(53));
        logic_layers[7]->get_link_by_num(29)->set_other_side_link(logic_layers[4]->get_link_by_num(19));
        logic_layers[7]->get_link_by_num(30)->set_other_side_link(logic_layers[5]->get_link_by_num(23));
        logic_layers[7]->get_link_by_num(31)->set_other_side_link(logic_layers[6]->get_link_by_num(26));

        logic_layers[8]->get_link_by_num(33)->set_other_side_link(logic_layers[9]->get_link_by_num(36));
        logic_layers[8]->get_link_by_num(34)->set_other_side_link(logic_layers[10]->get_link_by_num(43));
        logic_layers[8]->get_link_by_num(35)->set_other_side_link(logic_layers[11]->get_link_by_num(46));

        logic_layers[9]->get_link_by_num(36)->set_other_side_link(logic_layers[8]->get_link_by_num(33));
        logic_layers[9]->get_link_by_num(37)->set_other_side_link(logic_layers[3]->get_link_by_num(12));
        logic_layers[9]->get_link_by_num(38)->set_other_side_link(logic_layers[10]->get_link_by_num(40));
        logic_layers[9]->get_link_by_num(39)->set_other_side_link(logic_layers[11]->get_link_by_num(47));

        logic_layers[10]->get_link_by_num(40)->set_other_side_link(logic_layers[9]->get_link_by_num(38));
        logic_layers[10]->get_link_by_num(41)->set_other_side_link(logic_layers[6]->get_link_by_num(25));
        logic_layers[10]->get_link_by_num(42)->set_other_side_link(logic_layers[11]->get_link_by_num(44));
        logic_layers[10]->get_link_by_num(43)->set_other_side_link(logic_layers[8]->get_link_by_num(34));

        logic_layers[11]->get_link_by_num(44)->set_other_side_link(logic_layers[10]->get_link_by_num(42));
        logic_layers[11]->get_link_by_num(45)->set_other_side_link(logic_layers[15]->get_link_by_num(61));
        logic_layers[11]->get_link_by_num(46)->set_other_side_link(logic_layers[8]->get_link_by_num(35));
        logic_layers[11]->get_link_by_num(47)->set_other_side_link(logic_layers[9]->get_link_by_num(39));

        logic_layers[12]->get_link_by_num(49)->set_other_side_link(logic_layers[13]->get_link_by_num(52));
        logic_layers[12]->get_link_by_num(50)->set_other_side_link(logic_layers[14]->get_link_by_num(59));
        logic_layers[12]->get_link_by_num(51)->set_other_side_link(logic_layers[15]->get_link_by_num(62));

        logic_layers[13]->get_link_by_num(52)->set_other_side_link(logic_layers[12]->get_link_by_num(49));
        logic_layers[13]->get_link_by_num(53)->set_other_side_link(logic_layers[7]->get_link_by_num(28));
        logic_layers[13]->get_link_by_num(54)->set_other_side_link(logic_layers[14]->get_link_by_num(56));
        logic_layers[13]->get_link_by_num(55)->set_other_side_link(logic_layers[15]->get_link_by_num(63));

        logic_layers[14]->get_link_by_num(56)->set_other_side_link(logic_layers[13]->get_link_by_num(54));
        logic_layers[14]->get_link_by_num(57)->set_other_side_link(logic_layers[2]->get_link_by_num(9));
        logic_layers[14]->get_link_by_num(58)->set_other_side_link(logic_layers[15]->get_link_by_num(60));
        logic_layers[14]->get_link_by_num(59)->set_other_side_link(logic_layers[12]->get_link_by_num(50));

        logic_layers[15]->get_link_by_num(60)->set_other_side_link(logic_layers[14]->get_link_by_num(58));
        logic_layers[15]->get_link_by_num(61)->set_other_side_link(logic_layers[11]->get_link_by_num(45));
        logic_layers[15]->get_link_by_num(62)->set_other_side_link(logic_layers[12]->get_link_by_num(51));
        logic_layers[15]->get_link_by_num(63)->set_other_side_link(logic_layers[13]->get_link_by_num(55));
    }
}

// return link id for PIM
int Memory<HMC, Controller>::get_route_link_Mesh(int cur_cub, int target_cub)
{
    if (cur_cub == target_cub)
        return -1;

    if (stacks == 16)
    {
        if (cur_cub == 0)
        {
            if (target_cub == 4 || target_cub == 8 || target_cub == 12)
                return 2;
            else
                return 1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 1)
        {
            if (target_cub == 0 || target_cub == 4 || target_cub == 8 || target_cub == 12)
                return 4;
            else if (target_cub == 5 || target_cub == 9 || target_cub == 13)
                return 7;
            else
                return 6;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 2)
        {
            if (target_cub == 3 || target_cub == 7 || target_cub == 11 || target_cub == 15)
                return 10;
            else if (target_cub == 6 || target_cub == 10 || target_cub == 14)
                return 11;
            else
                return 8;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 3)
        {
            if (target_cub == 7 || target_cub == 11 || target_cub == 15)
                return 14;
            else
                return 15;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 4)
        {
            if (target_cub == 0)
                return 16;
            else if (target_cub == 8 || target_cub == 12)
                return 18;
            else
                return 17;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 5)
        {
            if (target_cub == 0 || target_cub == 4 || target_cub == 8 || target_cub == 12)
                return 20;
            else if (target_cub == 1)
                return 21;
            else if (target_cub == 9 || target_cub == 13)
                return 23;
            else
                return 22;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 6)
        {
            if (target_cub == 3 || target_cub == 7 || target_cub == 11 || target_cub == 15)
                return 26;
            else if (target_cub == 2)
                return 25;
            else if (target_cub == 10 || target_cub == 14)
                return 27;
            else
                return 24;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 7)
        {
            if (target_cub == 3)
                return 29;
            else if (target_cub == 11 || target_cub == 15)
                return 31;
            else
                return 28;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 8)
        {
            if (target_cub == 12)
                return 34;
            else if (target_cub == 0 || target_cub == 4)
                return 32;
            else
                return 33;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 9)
        {
            if (target_cub == 13)
                return 39;
            else if (target_cub == 1 || target_cub == 5)
                return 37;
            else if (target_cub == 0 || target_cub == 4 || target_cub == 8 || target_cub == 12)
                return 36;
            else
                return 38;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 10)
        {
            if (target_cub == 14)
                return 43;
            else if (target_cub == 2 || target_cub == 6)
                return 41;
            else if (target_cub == 3 || target_cub == 7 || target_cub == 11 || target_cub == 15)
                return 42;
            else
                return 40;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 11)
        {
            if (target_cub == 15)
                return 47;
            else if (target_cub == 3 || target_cub == 7)
                return 45;
            else
                return 44;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 12)
        {
            if (target_cub == 0 || target_cub == 4 || target_cub == 8)
                return 49;
            else
                return 50;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 13)
        {
            if (target_cub == 0 || target_cub == 4 || target_cub == 8 || target_cub == 12)
                return 52;
            else if (target_cub == 1 || target_cub == 5 || target_cub == 9)
                return 53;
            else
                return 54;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 14)
        {
            if (target_cub == 2 || target_cub == 6 || target_cub == 10)
                return 57;
            else if (target_cub == 3 || target_cub == 7 || target_cub == 11 || target_cub == 15)
                return 58;
            else
                return 56;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 15)
        {
            if (target_cub == 3 || target_cub == 7 || target_cub == 11)
                return 63;
            else
                return 62;
            // assert(0 && "error target cube id!");
        }
    }else if (stacks == 8)
    {
        if (cur_cub == 0)
        {
            if (target_cub == 4)
                return 2;
            else
                return 1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 1)
        {
            if (target_cub == 0 || target_cub == 4)
                return 4;
            else if (target_cub == 5)
                return 7;
            else
                return 6;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 2)
        {
            if (target_cub == 3 || target_cub == 7)
                return 10;
            else if (target_cub == 6)
                return 11;
            else
                return 8;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 3)
        {
            if (target_cub == 7)
                return 14;
            else
                return 15;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 4)
        {
            if (target_cub == 0)
                return 17;
            else
                return 18;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 5)
        {
            if (target_cub == 0 || target_cub == 4)
                return 20;
            else if (target_cub == 1)
                return 21;
            else
                return 22;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 6)
        {
            if (target_cub == 3 || target_cub == 7)
                return 26;
            else if (target_cub == 2)
                return 25;
            else
                return 24;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 7)
        {
            if (target_cub == 3)
                return 31;
            else
                return 30;
            // assert(0 && "error target cube id!");
        }
    }
    else if (stacks == 4)
    {
        if (cur_cub == 0)
        {
            if (target_cub == 2)
                return 2;
            else
                return 1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 1)
        {
            if (target_cub == 3)
                return 6;
            else
                return 7;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 2)
        {
            if (target_cub == 0)
                return 9;
            else
                return 10;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 3)
        {
            if (target_cub == 1)
                return 15;
            else
                return 14;
            // assert(0 && "error target cube id!");
        }
    }
    else
    {
        assert(0 && "Only support 16 stacks for Mesh topology!");
        return -1;
    }
}

int Memory<HMC, Controller>::get_route_link_Dragonfly(int cur_cub, int target_cub)
{
    if (stacks == 4)
    {
        if (cur_cub == 0)
        {
            if (target_cub == 1)
                return 1;
            else if (target_cub == 2)
                return 3;
            else
                return 2;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 1)
        {
            if (target_cub == 0)
                return 7;
            else if (target_cub == 2)
                return 6;
            else
                return 5;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 2)
        {
            if (target_cub == 0)
                return 9;
            else if (target_cub == 1)
                return 10;
            else
                return 11;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 3)
        {
            if (target_cub == 0)
                return 14;
            else if (target_cub == 1)
                return 15;
            else
                return 13;
            // assert(0 && "error target cube id!");
        }
    }
    else if (stacks == 8)
    {
        if (cur_cub == 0)
        {
            if (target_cub == 1)
                return 1;
            else if (target_cub == 2)
                return 2;
            else if (target_cub == 3)
                return 3;
            else if (target_cub == 4 || target_cub == 5)
                return 1;
            else if (target_cub == 6 || target_cub == 7)
                return 2;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 1)
        {
            if (target_cub == 0)
                return 4;
            else if (target_cub == 2)
                return 6;
            else if (target_cub == 3)
                return 7;
            else if (target_cub >= 4 && target_cub <= 7)
                return 5;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 2)
        {
            if (target_cub == 0)
                return 11;
            else if (target_cub == 1)
                return 8;
            else if (target_cub == 3)
                return 10;
            else if (target_cub >= 4 && target_cub <= 7)
                return 9;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 3)
        {
            if (target_cub == 0)
                return 13;
            else if (target_cub == 1)
                return 14;
            else if (target_cub == 2)
                return 15;
            else if (target_cub == 4 || target_cub == 5)
                return 14;
            else if (target_cub == 6 || target_cub == 7)
                return 15;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 4)
        {
            if (target_cub == 5)
                return 17;
            else if (target_cub == 6)
                return 18;
            else if (target_cub == 7)
                return 19;
            else if (target_cub == 0 || target_cub == 1)
                return 17;
            else if (target_cub == 2 || target_cub == 3)
                return 18;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 5)
        {
            if (target_cub == 4)
                return 20;
            else if (target_cub == 6)
                return 22;
            else if (target_cub == 7)
                return 23;
            else if (target_cub >= 0 && target_cub <= 3)
                return 21;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 6)
        {
            if (target_cub == 4)
                return 27;
            else if (target_cub == 5)
                return 24;
            else if (target_cub == 7)
                return 26;
            else if (target_cub >= 0 && target_cub <= 3)
                return 25;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 7)
        {
            if (target_cub == 4)
                return 29;
            else if (target_cub == 5)
                return 30;
            else if (target_cub == 6)
                return 31;
            else if (target_cub == 0 || target_cub == 1)
                return 30;
            else if (target_cub == 2 || target_cub == 3)
                return 31;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
    }
    else if (stacks == 16)
    {
        if (cur_cub == 0)
        {
            if (target_cub == 1)
                return 1;
            else if (target_cub == 2)
                return 2;
            else if (target_cub == 3)
                return 3;
            else if (target_cub >= 4 && target_cub <= 7)
                return 1;
            else if (target_cub >= 8 && target_cub <= 11)
                return 3;
            else if (target_cub >= 12 && target_cub <= 15)
                return 2;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 1)
        {
            if (target_cub == 0)
                return 4;
            else if (target_cub == 2)
                return 6;
            else if (target_cub == 3)
                return 7;
            else if (target_cub >= 4 && target_cub <= 7)
                return 5;
            else if (target_cub >= 8 && target_cub <= 11)
                return 7;
            else if (target_cub >= 12 && target_cub <= 15)
                return 6;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 2)
        {
            if (target_cub == 0)
                return 11;
            else if (target_cub == 1)
                return 8;
            else if (target_cub == 3)
                return 10;
            else if (target_cub >= 4 && target_cub <= 7)
                return 8;
            else if (target_cub >= 8 && target_cub <= 11)
                return 10;
            else if (target_cub >= 12 && target_cub <= 15)
                return 9;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 3)
        {
            if (target_cub == 0)
                return 13;
            else if (target_cub == 1)
                return 14;
            else if (target_cub == 2)
                return 15;
            else if (target_cub >= 4 && target_cub <= 7)
                return 14;
            else if (target_cub >= 8 && target_cub <= 11)
                return 12;
            else if (target_cub >= 12 && target_cub <= 15)
                return 15;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 4)
        {
            if (target_cub == 5)
                return 17;
            else if (target_cub == 6)
                return 18;
            else if (target_cub == 7)
                return 19;
            else if (target_cub >= 0 && target_cub <= 3)
                return 17;
            else if (target_cub >= 8 && target_cub <= 11)
                return 18;
            else if (target_cub >= 12 && target_cub <= 15)
                return 19;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 5)
        {
            if (target_cub == 4)
                return 20;
            else if (target_cub == 6)
                return 22;
            else if (target_cub == 7)
                return 23;
            else if (target_cub >= 0 && target_cub <= 3)
                return 21;
            else if (target_cub >= 8 && target_cub <= 11)
                return 22;
            else if (target_cub >= 12 && target_cub <= 15)
                return 23;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 6)
        {
            if (target_cub == 4)
                return 27;
            else if (target_cub == 5)
                return 24;
            else if (target_cub == 7)
                return 26;
            else if (target_cub >= 0 && target_cub <= 3)
                return 24;
            else if (target_cub >= 8 && target_cub <= 11)
                return 25;
            else if (target_cub >= 12 && target_cub <= 15)
                return 26;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 7)
        {
            if (target_cub == 4)
                return 29;
            else if (target_cub == 5)
                return 30;
            else if (target_cub == 6)
                return 31;
            else if (target_cub >= 0 && target_cub <= 3)
                return 30;
            else if (target_cub >= 8 && target_cub <= 11)
                return 31;
            else if (target_cub >= 12 && target_cub <= 15)
                return 28;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 8)
        {
            if (target_cub == 9)
                return 33;
            else if (target_cub == 10)
                return 34;
            else if (target_cub == 11)
                return 35;
            else if (target_cub >= 0 && target_cub <= 3)
                return 33;
            else if (target_cub >= 4 && target_cub <= 7)
                return 34;
            else if (target_cub >= 12 && target_cub <= 15)
                return 35;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 9)
        {
            if (target_cub == 8)
                return 36;
            else if (target_cub == 10)
                return 38;
            else if (target_cub == 11)
                return 39;
            else if (target_cub >= 0 && target_cub <= 3)
                return 37;
            else if (target_cub >= 4 && target_cub <= 7)
                return 38;
            else if (target_cub >= 12 && target_cub <= 15)
                return 39;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 10)
        {
            if (target_cub == 8)
                return 43;
            else if (target_cub == 9)
                return 40;
            else if (target_cub == 11)
                return 42;
            else if (target_cub >= 0 && target_cub <= 3)
                return 40;
            else if (target_cub >= 4 && target_cub <= 7)
                return 41;
            else if (target_cub >= 12 && target_cub <= 15)
                return 42;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 11)
        {
            if (target_cub == 8)
                return 46;
            else if (target_cub == 9)
                return 47;
            else if (target_cub == 10)
                return 44;
            else if (target_cub >= 0 && target_cub <= 3)
                return 47;
            else if (target_cub >= 4 && target_cub <= 7)
                return 44;
            else if (target_cub >= 12 && target_cub <= 15)
                return 45;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 12)
        {
            if (target_cub == 13)
                return 49;
            else if (target_cub == 14)
                return 50;
            else if (target_cub == 15)
                return 51;
            else if (target_cub >= 0 && target_cub <= 3)
                return 50;
            else if (target_cub >= 4 && target_cub <= 7)
                return 49;
            else if (target_cub >= 8 && target_cub <= 11)
                return 51;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 13)
        {
            if (target_cub == 12)
                return 52;
            else if (target_cub == 14)
                return 54;
            else if (target_cub == 15)
                return 55;
            else if (target_cub >= 0 && target_cub <= 3)
                return 54;
            else if (target_cub >= 4 && target_cub <= 7)
                return 53;
            else if (target_cub >= 8 && target_cub <= 11)
                return 55;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 14)
        {
            if (target_cub == 12)
                return 59;
            else if (target_cub == 13)
                return 56;
            else if (target_cub == 15)
                return 58;
            else if (target_cub >= 0 && target_cub <= 3)
                return 57;
            else if (target_cub >= 4 && target_cub <= 7)
                return 56;
            else if (target_cub >= 8 && target_cub <= 11)
                return 58;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
        else if (cur_cub == 15)
        {
            if (target_cub == 12)
                return 62;
            else if (target_cub == 13)
                return 63;
            else if (target_cub == 14)
                return 60;
            else if (target_cub >= 0 && target_cub <= 3)
                return 60;
            else if (target_cub >= 4 && target_cub <= 7)
                return 63;
            else if (target_cub >= 8 && target_cub <= 11)
                return 61;
            else
                return -1;
            // assert(0 && "error target cube id!");
        }
    }
    return -1;
}

}