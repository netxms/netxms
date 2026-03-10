import * as esbuild from 'esbuild';
import * as path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const outfile = path.resolve(__dirname,
   '../../src/client/nxmc/java/src/rwt/resources/js/codemirror/codemirror-nxsl.min.js');

esbuild.build({
   entryPoints: [path.resolve(__dirname, 'src/index.js')],
   bundle: true,
   minify: true,
   keepNames: true,
   format: 'iife',
   globalName: '__codemirror_unused__',
   outfile: outfile,
   target: ['es2018'],
   logLevel: 'info',
}).then(() => {
   console.log(`Bundle written to ${outfile}`);
}).catch(() => process.exit(1));
