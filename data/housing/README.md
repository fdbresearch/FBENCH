# Housing Dataset

This dataset is generated to provide a scalable example exhibiting very high compression using factorised representations. The generated relations have the following schemas:

* House(postcode,livingarea,price,nbbedrooms,nbbathrooms,kitchensize,house,flat,condo,garden,parking)
* Shop(postcode,openinghoursshop,pricerangeshop,sainsburys,tesco,ms)
* Institution(postcode,typeeducation,sizeinstitution)
* Restaurant(postcode,openinghoursrest,pricerangerest)
* Demographics(postcode,averagesalary,crimesperyear,unemployment,nbhospitals)
* Transport(postcode,nbbuslines,nbtrainstations,distancecitycentre)

## Data Generation

The data is generated along the following parameters:

### Number of tuples in the Relations

Regardless of the scaling factor ´s´ we always have
* 25000 postcodes
* 1 Transport-Tuple per postcode
* 1 Demographics-Tuple per postcode

Given a scaling factor `s` the following amount of tuples is generated:
* House: `s` per postcode
* Education: `4 log_2(s+1)` per postcode
* Hospital: `2 log_2(s+1)` per postcode
* Shop: `s` per postcode
* Restaurant: `(s+1)/2` per postcode
* Entertainment: `2 log_2(s+1)` per postcode

### Data ranges/Domains

The data is generated from the following data ranges:

House
* postcode - `[1,25000]`
* livingarea - float `[10, 200]`
* price - float `[10000, 10000000]`
* nbbedrooms - int `[1,10]`
* nbbathrooms - int `[1,10]`
* kitchensize - float `[0,50]`
* house - int `[0,1]` (yes/no)
* flat - int `[0,1]` (yes/no)
* otherhouse - int `[0,1]` (yes/no)
* garden - float `[0,10000]` (sqm)
* parking - int `[0,10]` (nb of parking lots)


Education
* postcode
* type - int `[1,3]` (public/private/independent)
* level - int `[1,4]` (preschool/primary/secondary/university)


Hospital
* postcode
* type - int `[0,1]` (public/private)
* emergency - int `[0,1]` (yes/no)


Shop
* postcode
* openinghours - int `[8,24]` (nb of hours it is opened per day)
* pricerange - int `[0,2]` (cheap/medium/expensive)
* sainsburys - int `[0,1]` (yes/no)
* tesco - int `[0,1]` (yes/no)
* ms - int `[0,1]` (yes/no)


Restaurant
* postcode
* openinghours - int `[8,24]` (nb of hours it is opened per day)
* pricerange - int `[0,2]` (cheap/medium/expensive)



Entertainment
* postcode
* cinema - int `[0,1]` (yes/no)
* museum - int `[0,1]` (yes/no)


Transport
* postcode
* nbbuslines - int `[1,20]`
* nbtrainstations - int `[1,20]`
* distancecitycentre - float `[0,100]`
* priceperticket - float `[1,10]`


Demographics
* postcode
* averagesalary - float `[10000,100000]`
* crimesperyear - float `[0,1000]`
* unemployment - float `[0,100]`

## Compression

The query used is the natural join over all the relations (joining on postcode). By design of the data, at scaling factor `s`, the size of the listing representation is ca. `200k * s^3 * (log_2(s+1))^3`tuples. The factorised size on the other hand is ca. `75k * (s + 3 * log_2 (s+1))`. We generated the data for scale factors 1 to 20, leading to the following results:

Scale Factor | Tuples | Values in the listing representation | Values in the factorised representation| Compression Factor
-------------|-------:|-------------------------------------:|---------------------------------------:|------------------:
1  | 25,000      | 675,000        | 675,000   | 1.00
2  | 100,000     | 2,700,000      | 1,004,885 | 2.69
3  | 899,586     | 24,288,822     | 1,396,360 | 17.39
4  | 1,599,264   | 43,180,128     | 1,678,591 | 25.72
5  | 3,748,275   | 101,203,425    | 1,983,375 | 51.03
6  | 5,397,516   | 145,732,932    | 2,241,566 | 65.01
7  | 14,687,064  | 396,550,728    | 2,569,455 | 154.33
8  | 19,189,248  | 518,109,696    | 2,812,929 | 184.19
9  | 30,351,915  | 819,501,705    | 3,080,946 | 265.99
10 | 37,470,000  | 1,011,690,000  | 3,315,615 | 305.13
11 | 54,405,714  | 1,468,954,278  | 3,574,277 | 410.98
12 | 64,756,800  | 1,748,433,600  | 3,800,425 | 460.06
13 | 88,661,118  | 2,393,850,186  | 4,050,426 | 591.01
14 | 102,827,284 | 2,776,336,668  | 4,271,797 | 649.92
15 | 179,767,800 | 4,853,730,600  | 4,560,677 | 1064.26
16 | 204,533,760 | 5,522,411,520  | 4,776,876 | 1156.07
17 | 259,720,254 | 7,012,446,858  | 5,017,126 | 1397.70
18 | 291,322,980 | 7,865,720,460  | 5,230,274 | 1503.88
19 | 360,541,530 | 9,734,621,310  | 5,467,036 | 1780.60
20 | 399,500,000 | 10,786,500,000 | 5,677,422 | 1899.89
