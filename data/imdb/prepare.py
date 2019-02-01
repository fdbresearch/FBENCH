import csv

tconst_dict = {}
nconst_dict = {}
title_dict = {'\\N': -1}
region_dict = {'\\N': -1}
language_dict = {'\\N': -1}
titleType_dict = {'\\N': -1}
names_dict = {'\\N': -1}
category_dict = {'\\N': -1}
job_dict = {'\\N': -1}
characters_dict = {'\\N': -1}

types_dict = {}
attributes_dict = {}
genres_dict = {}
profession_dict = {}

def main():
    title_akas("title.akas")
    title_basics("title.basics")
    title_crew("title.crew")
    title_episode("title.episode")
    title_principals("title.principals")
    title_ratings("title.ratings")
    name_basics("name.basics")

def test():
    title_akas("title.akas.test")
    title_basics("title.basics.test")
    title_crew("title.crew.test")
    title_episode("title.episode.test")
    title_principals("title.principals.test")
    title_ratings("title.ratings.test")
    name_basics("name.basics.test")

def title_akas(filename):
    csvfile = open(filename+".tsv", 'r')
    csvout = open(filename+".tbl", 'w')
    rdr = csv.reader(csvfile, delimiter="\t", quoting=csv.QUOTE_NONE)
    wrtr = csv.writer(csvout, delimiter="|", quoting=csv.QUOTE_MINIMAL)
    types_wrtr = csv.writer(open(filename+"_types.tbl", 'w'), delimiter="|", quoting=csv.QUOTE_MINIMAL)
    attributes_wrtr = csv.writer(open(filename+"_attributes.tbl", 'w'), delimiter="|", quoting=csv.QUOTE_MINIMAL)
    rdr.next() # First row is header -> ignore
    for row in rdr:
        outrow = []
        if not tconst_dict.has_key(row[0]):
            tconst_dict[row[0]] = len(tconst_dict)
        if not title_dict.has_key(row[2]):
            title_dict[row[2]] = len(title_dict)
        if not region_dict.has_key(row[3]):
            region_dict[row[3]] = len(region_dict)
        if not language_dict.has_key(row[4]):
            language_dict[row[4]] = len(language_dict)
        outrow.append(tconst_dict[row[0]])
        outrow.append(int(row[1]))
        outrow.append(title_dict[row[2]])
        outrow.append(region_dict[row[3]])
        outrow.append(language_dict[row[4]])
        if row[5] != '\\N':
            for t in row[5].split(","):
                if not types_dict.has_key(t):
                    types_dict[t] = len(types_dict)
                types_wrtr.writerow([tconst_dict[row[0]], int(row[1]), types_dict[t]])
        if row[6] != '\\N':
            for a in row[6].split(","):
                if not attributes_dict.has_key(a):
                    attributes_dict[a] = len(attributes_dict)
                attributes_wrtr.writerow([tconst_dict[row[0]], int(row[1]), attributes_dict[a]])
        outrow.append(int(row[7]) if row[7] != '\\N' else -1)
        wrtr.writerow(outrow)
    csvfile.close()
    csvout.close()

def title_basics(filename):
    csvfile = open(filename+".tsv", 'r')
    csvout = open(filename+".tbl", 'w')
    rdr = csv.reader(csvfile, delimiter="\t", quoting=csv.QUOTE_NONE)
    wrtr = csv.writer(csvout, delimiter="|", quoting=csv.QUOTE_MINIMAL)
    genres_wrtr = csv.writer(open(filename+"_genres.tbl", 'w'), delimiter="|", quoting=csv.QUOTE_MINIMAL)
    rdr.next() # First row is header -> ignore
    for row in rdr:
        outrow = []
        if not tconst_dict.has_key(row[0]):
            tconst_dict[row[0]] = len(tconst_dict)
        if not titleType_dict.has_key(row[1]):
            titleType_dict[row[1]] = len(titleType_dict)
        if not title_dict.has_key(row[2]):
            title_dict[row[2]] = len(title_dict)
        if not title_dict.has_key(row[3]):
            title_dict[row[3]] = len(title_dict)
        outrow.append(tconst_dict[row[0]])
        outrow.append(titleType_dict[row[1]])
        outrow.append(title_dict[row[2]])
        outrow.append(title_dict[row[3]])
        outrow.append(int(row[4]) if row[4] != '\\N' else -1)
        outrow.append(int(row[5]) if row[5] != '\\N' else -1)
        outrow.append(int(row[6]) if row[6] != '\\N' else -1)
        outrow.append(int(row[7]) if row[7] != '\\N' else -1)
        if row[8] != '\\N':
            for g in row[8].split(","):
                if not genres_dict.has_key(g):
                    genres_dict[g] = len(genres_dict)
                genres_wrtr.writerow([tconst_dict[row[0]], genres_dict[g]])
        wrtr.writerow(outrow)
    csvfile.close()
    csvout.close()

def title_crew(filename):
    csvfile = open(filename+".tsv", 'r')
    csvout_dir = open(filename+"_directors.tbl", 'w')
    csvout_wri = open(filename+"_writers.tbl", 'w')
    rdr = csv.reader(csvfile, delimiter="\t", quoting=csv.QUOTE_NONE)
    directors_wrtr = csv.writer(csvout_dir, delimiter="|", quoting=csv.QUOTE_MINIMAL)
    writers_wrtr = csv.writer(csvout_wri, delimiter="|", quoting=csv.QUOTE_MINIMAL)
    rdr.next() # First row is header -> ignore
    for row in rdr:
        if not tconst_dict.has_key(row[0]):
            tconst_dict[row[0]] = len(tconst_dict)
        if row[1] != '\\N':
            for d in row[1].split(","):
                if not nconst_dict.has_key(d):
                    nconst_dict[d] = len(nconst_dict)
                directors_wrtr.writerow([tconst_dict[row[0]], nconst_dict[d]])
        if row[2] != '\\N':
            for w in row[2].split(","):
                if not nconst_dict.has_key(w):
                    nconst_dict[w] = len(nconst_dict)
                writers_wrtr.writerow([tconst_dict[row[0]], nconst_dict[w]])
    csvfile.close()
    csvout_dir.close()
    csvout_wri.close()

def title_episode(filename):
    csvfile = open(filename+".tsv", 'r')
    csvout = open(filename+".tbl", 'w')
    rdr = csv.reader(csvfile, delimiter="\t", quoting=csv.QUOTE_NONE)
    wrtr = csv.writer(csvout, delimiter="|", quoting=csv.QUOTE_MINIMAL)
    rdr.next() # First row is header -> ignore
    for row in rdr:
        outrow = []
        if not tconst_dict.has_key(row[0]):
            tconst_dict[row[0]] = len(tconst_dict)
        if not tconst_dict.has_key(row[1]):
            tconst_dict[row[1]] = len(tconst_dict)
        outrow.append(tconst_dict[row[0]])
        outrow.append(tconst_dict[row[1]])
        outrow.append(int(row[2]) if row[2] != "\\N" else -1)
        outrow.append(int(row[3]) if row[2] != "\\N" else -1)
        wrtr.writerow(outrow)
    csvfile.close()
    csvout.close()

def title_principals(filename):
    csvfile = open(filename+".tsv", 'r')
    csvout = open(filename+".tbl", 'w')
    rdr = csv.reader(csvfile, delimiter="\t", quoting=csv.QUOTE_NONE)
    wrtr = csv.writer(csvout, delimiter="|", quoting=csv.QUOTE_MINIMAL)
    rdr.next() # First row is header -> ignore
    for row in rdr:
        outrow = []
        if not tconst_dict.has_key(row[0]):
            tconst_dict[row[0]] = len(tconst_dict)
        if not nconst_dict.has_key(row[2]):
            nconst_dict[row[2]] = len(nconst_dict)
        if not category_dict.has_key(row[3]):
            category_dict[row[3]] = len(category_dict)
        if not job_dict.has_key(row[4]):
            job_dict[row[4]] = len(job_dict)
        if not characters_dict.has_key(row[5]):
            characters_dict[row[5]] = len(characters_dict)
        outrow.append(tconst_dict[row[0]])
        outrow.append(int(row[1]) if row[1] != '\\N' else -1)
        outrow.append(nconst_dict[row[2]])
        outrow.append(category_dict[row[3]])
        outrow.append(job_dict[row[4]])
        outrow.append(characters_dict[row[5]])
        wrtr.writerow(outrow)
    csvfile.close()
    csvout.close()

def title_ratings(filename):
    csvfile = open(filename+".tsv", 'r')
    csvout = open(filename+".tbl", 'w')
    rdr = csv.reader(csvfile, delimiter="\t", quoting=csv.QUOTE_NONE)
    wrtr = csv.writer(csvout, delimiter="|", quoting=csv.QUOTE_MINIMAL)
    rdr.next() # First row is header -> ignore
    for row in rdr:
        outrow = []
        if not tconst_dict.has_key(row[0]):
            tconst_dict[row[0]] = len(tconst_dict)
        outrow.append(tconst_dict[row[0]])
        outrow.append(float(row[1]))
        outrow.append(int(row[2]) if row[2] != '\\N' else -1)
        wrtr.writerow(outrow)
    csvfile.close()
    csvout.close()

def name_basics(filename):
    csvfile = open(filename+".tsv", 'r')
    csvout = open(filename+".tbl", 'w')
    rdr = csv.reader(csvfile, delimiter="\t", quoting=csv.QUOTE_NONE)
    wrtr = csv.writer(csvout, delimiter="|", quoting=csv.QUOTE_MINIMAL)
    profession_wrtr = csv.writer(open(filename+"_profession.tbl", 'w'), delimiter="|", quoting=csv.QUOTE_MINIMAL)
    knownFor_wrtr = csv.writer(open(filename+"_knownFor.tbl", 'w'), delimiter="|", quoting=csv.QUOTE_MINIMAL)
    rdr.next() # First row is header -> ignore
    for row in rdr:
        outrow = []
        if not nconst_dict.has_key(row[0]):
            nconst_dict[row[0]] = len(nconst_dict)
        if not names_dict.has_key(row[1]):
            names_dict[row[1]] = len(names_dict)
        outrow.append(nconst_dict[row[0]])
        outrow.append(names_dict[row[1]])
        outrow.append(int(row[2]) if row[2] != '\\N' else -1)
        outrow.append(int(row[3]) if row[3] != '\\N' else -1)
        if row[4] != '\\N':
            for p in row[4].split(","):
                if not profession_dict.has_key(p):
                    profession_dict[p] = len(profession_dict)
                profession_wrtr.writerow([nconst_dict[row[0]], profession_dict[p]])
        if row[5] != '\\N':
            for t in row[5].split(","):
                if not tconst_dict.has_key(t):
                    tconst_dict[t] = len(tconst_dict)
                knownFor_wrtr.writerow([nconst_dict[row[0]], tconst_dict[t]])
        wrtr.writerow(outrow)
    csvfile.close()
    csvout.close()

if __name__=="__main__":
    main()
