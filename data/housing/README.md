# Housing Dataset

The Dataset contains six relations with the following schemas:

* House(postcode,livingarea,price,nbbedrooms,nbbathrooms,kitchensize,house,flat,condo,garden,parking)
* Shop(postcode,openinghoursshop,pricerangeshop,sainsburys,tesco,ms)
* Institution(postcode,typeeducation,sizeinstitution)
* Restaurant(postcode,openinghoursrest,pricerangerest)
* Demographics(postcode,averagesalary,crimesperyear,unemployment,nbhospitals)
* Transport(postcode,nbbuslines,nbtrainstations,distancecitycentre)

The query used is a natural join of all relations on the attribute postcode. This query result compresses very well, the compression factor achieved is 1899.89.
