#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <signal.h>
#include <fcntl.h>
#include <cups/raster.h>
#include "rastertosharpgdi.h"
#include "packbits.h"

void generate_job(cups_page_header2_t *header) {
    sharp_cmd_t begin_job_hdr = { .magic = 0x8181, .cmd = SHARP_BEGIN_JOB, .length = 0 };
    sharp_cmd_t begin_job_desc_hdr = { .magic = 0x8181, .cmd = SHARP_BEGIN_JOB_DESCRIPTION };

    fwrite(&begin_job_hdr, sizeof(begin_job_hdr), 1, stdout);

    char jobstr[64];
    sprintf(jobstr, "200z0p0s%dr0j1o1x800000a0h1m0v0i0b0t600d%dc0e",
        header->HWResolution[0],
        header->Collate
    );
    begin_job_desc_hdr.length = htons(strlen(jobstr));

    fwrite(&begin_job_desc_hdr, sizeof(begin_job_desc_hdr), 1, stdout);
    fwrite(jobstr, ntohs(begin_job_desc_hdr.length), 1, stdout);
}

void generate_page(cups_page_header2_t *header) {
    sharp_cmd_t begin_page_hdr = { .magic = 0x8181, .cmd = SHARP_BEGIN_PAGE_DESCRIPTION };
    sharp_cmd_t begin_raster_hdr = { .magic = 0x8181, .cmd = SHARP_BEGIN_RASTER, .length = 0 };

    int page_size_code = 9;
    for (int i = 0; i < sizeof(sharp_paper_codes) / sizeof(sharp_paper_codes[0]); i++) {
        if (!strcmp(header->cupsPageSizeName, sharp_paper_codes[i].paper_name)) {
            page_size_code = sharp_paper_codes[i].paper_code;
        }
    }

    char pagestr[96];
    sprintf(pagestr, "%dh%da0d257p%dt%ds96e96l2m16v48u%do0r%df",
        header->MediaPosition,
        page_size_code,
        header->cupsHeight,
        header->cupsBytesPerLine * 8,
        0, /* portrait / landscape */
        0  /* 0 = portrait raster (t > s, rows are page-width),
              1 = landscape raster (s > t, rows are page-height; rotate 270° CCW to view upright) */
    );
    begin_page_hdr.length = htons(strlen(pagestr));
    fwrite(&begin_page_hdr, sizeof(begin_page_hdr), 1, stdout);
    fwrite(pagestr, ntohs(begin_page_hdr.length), 1, stdout);
    fwrite(&begin_raster_hdr, sizeof(begin_raster_hdr), 1, stdout);
}

int main(int argc, char *argv[], char *envp[]) {
    signal(SIGPIPE, SIG_IGN);
    
    if (argc < 6 || argc > 7) {
        fprintf(stderr, "ERROR: incorrect number of arguments.\n");
        fprintf(stderr, "ERROR: %s job-id user title copies options [file]\n", argv[0]);
        return 1;
    }

    int inputfile = 0;
    if (argc == 7) {    
        inputfile = open(argv[6], O_RDONLY);
        if (inputfile < 0) {
            fprintf(stderr, "ERROR: can't open input file.\n");
            return 1;
        }
    }
    cups_raster_t *ras = cupsRasterOpen(inputfile, CUPS_RASTER_READ);
    cups_page_header2_t header;
    int page = 0;
    int new_width, new_height;
    char *inbuffer;
    size_t inbuffer_size = 0;
    char *outbuffer;
    size_t outbuffer_size = 0;
    size_t pbsize = 0;
    sharp_cmd_t raster_hdr = { .magic = 0x8181, .cmd = SHARP_RASTER_DATA };
    sharp_cmd_t raster_end_hdr = { .magic = 0x8181, .cmd = SHARP_END_RASTER_DATA, .length = 0 };

    while (cupsRasterReadHeader2(ras, &header)) {
        page ++;
        if (! (header.cupsBitsPerColor == 1 &&
               header.cupsColorSpace == 3) ) {
                    fprintf(stderr, "ERROR: Can't process this raster data!\n");
                    return 2;
        }
        fprintf(stderr, "PAGE: %d %d\n", page, header.NumCopies);

	    inbuffer_size = header.cupsBytesPerLine * header.cupsHeight;
	    outbuffer_size = inbuffer_size  + 1*1024*1024;
        inbuffer = malloc(inbuffer_size);
        outbuffer = malloc(outbuffer_size);

        if (!inbuffer || !outbuffer) {
            fprintf(stderr, "ERROR: Can't allocate buffers!\n");
            return 3;
        }

        if (cupsRasterReadPixels(ras, inbuffer, inbuffer_size)) {
            pbsize = packbits(inbuffer, outbuffer, inbuffer_size, outbuffer_size);
        }

        if (!pbsize) {
            fprintf(stderr, "ERROR: PackBits incorrect output!\n");
            return 4;
        }

        if (page == 1) {
            generate_job(&header);
        }

        for (int copy = 0; copy < header.NumCopies; copy++) {
            generate_page(&header);

            int i;
            for (i = 0; i < pbsize; i += SHARP_PB_CHUNK) {
                raster_hdr.length = htons(pbsize - i < SHARP_PB_CHUNK ? pbsize - i : SHARP_PB_CHUNK);
                //fprintf(stderr, "! %d (%d / %d)\n", i, i % SHARP_PB_CHUNK, ntohs(raster_hdr.length));
                fwrite(&raster_hdr, sizeof(raster_hdr), 1, stdout);
                fwrite(outbuffer + i, ntohs(raster_hdr.length), 1, stdout);
            }
            //fprintf(stderr, "pbsize = %d, i = %d\n", pbsize, i);
            fwrite(&raster_end_hdr, sizeof(raster_end_hdr), 1, stdout);
        }

        free(inbuffer);
        free(outbuffer);
    }

    if (page) {
        sharp_cmd_t end_job_hdr = { .magic = 0x8181, .cmd = SHARP_END_JOB, .length = 0 };
        sharp_cmd_t end_job_finished_hdr = { .magic = 0x8181, .cmd = SHARP_FINISHED, .length = 0 };
        fwrite(&end_job_hdr, sizeof(end_job_hdr), 1, stdout);
        fwrite(&end_job_finished_hdr, sizeof(end_job_finished_hdr), 1, stdout);
    }


}