import os
import git
import errno
import shutil
import sqlite3

db = 'storage.db'

def get_repo_url(wd):
    try:
        name = git.Repo(wd).remote().config_reader.get('url')
    except git.InvalidGitRepositoryError as e:
        name = ''
    return name

class GitPollerException(Exception):
    def __init__(self, message):
        super(GitPollerException, self).__init__()
        self.message = message

    def __str__(self):
        return str(self.message)

class GitPollerHeads(object):
    git = git.Git()
    def __init__(self,
            name,
            repourl,
            branches=None,
            workdir=None,
            force=False):
        self.name = name
        self.repourl = repourl
        self.db = sqlite3.connect(db)
        self.branches = [branches] if isinstance(branches, basestring) else branch
        self.workdir = workdir
        if self.workdir:
            while True:
                try:
                    self.repo = git.Repo.clone_from(self.repourl, self.workdir, bare=True)
                    break
                except (OSError, git.GitCommandError) as e:
                    if (hasattr(e, 'errno') and e.errno == errno.EEXIST or
                            hasattr (e, 'message') and e.stderr.find('already exists') != -1):
                        if not get_repo_url(self.workdir) or force:
                            shutil.rmtree(self.workdir)
                            break
                        elif get_repo_url(self.workdir) == self.repourl:
                            self.repo = git.Repo(self.workdir)
                            break
                        else:
                            raise GitPollerException('There\'s already directory %s with git-repo, that points on %s, that differs from %s' % \
                                    (repr(self.workdir), repr(get_repo_url(self.workdir)), repr(self.repourl)))
                    else:
                        raise
        if not len(self.db.execute('''SELECT name FROM sqlite_master WHERE type='table' AND name='%s' ''' % self.name).fetchmany()):
            self.init_table()

    def get_remotes(self):
        d = [line.split('\t') for line in self.git.ls_remote('--heads', self.repourl).split('\n') if line]
        d = {line[0]: line[1] for line in [self._filterHeads(_line) for _line in d] if (self.branches is None or line[0] in self.branches)}
        return d

    def init_table(self):
        print self.name
        self.db.execute('''\
CREATE TABLE '%s'(
    branch TEXT,
    sha1   TEXT,
    PRIMARY KEY (branch ASC))\
''' % self.name)

    def _filterHeads(self, pair):
        return [unicode(pair[1][11:]), unicode(pair[0])]

    def _filterTags(self, pair):
        if pair[1].find('^{}') != -1:
            return []
        return [unicode(pair[1][10:]), unicode(pair[0])]

    def check(self, commits=False):
        branches_pre = dict(self.db.execute('''SELECT * FROM '%s' ''' % self.name).fetchall())
        branches_new = self.get_remotes()
        deleted = {a: branches_pre[a] for a in set(branches_pre.keys()) - set(branches_new.keys())}
        new     = {a: branches_new[a] for a in set(branches_new.keys()) - set(branches_pre.keys())}
        updated = {a: (branches_new[a], branches_pre[a]) for a in set(branches_new.keys()) & set(branches_pre.keys()) if \
                branches_new[a] != branches_pre[a]}
        if commits and self.workdir:
            self.repo.git.fetch(prune=True)

        for i in deleted:
            self.db.execute('''delete from '%s' where branch='%s' ''' % (self.name, i))
        for i in new:
            self.db.execute('''insert into '%s' values ('%s', '%s')''' % (self.name, i, new[i]))
        for i in updated:
            self.db.execute('''update '%s' set sha1 = '%s' where branch='%s' ''' % (self.name, i, updated[i][0]))
        self.db.commit()
        if commits and self.workdir:
            deleted = {branch: self.repo.commit(sha1) for branch, sha1 in deleted.iteritems()}
            new     = {branch: self.repo.commit(sha1) for branch, sha1 in     new.iteritems()}
            updated = {branch: self.repo.commit(sha1) for branch, sha1 in updated.iteritems()}
        return deleted, new, updated
